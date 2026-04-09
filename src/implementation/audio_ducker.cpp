#include "audio_ducker.h"

#include <QSettings>
#include <QTimer>

#ifdef _WIN32
#include <windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <combaseapi.h>
#include <endpointvolume.h>
#include <propsys.h>
#include <propvarutil.h>
#endif

namespace {
constexpr int kAudioPollIntervalMs = 100;
constexpr int kVolumeRampIntervalMs = 60;
constexpr int kVolumeRampDurationMs = 420;
constexpr auto kSettingsGroup = "audioDucker";
#ifdef _WIN32
MIDL_INTERFACE("C02216F6-8C67-4B5B-9D00-D008E73E0064")
IAudioMeterInformationLocal : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetPeakValue(float *peak) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMeteringChannelCount(UINT *channelCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetChannelsPeakValues(UINT channelCount, float *peakValues) = 0;
    virtual HRESULT STDMETHODCALLTYPE QueryHardwareSupport(DWORD *hardwareSupportMask) = 0;
};

const IID kAudioMeterInformationLocalIid = {
    0xC02216F6, 0x8C67, 0x4B5B, {0x9D, 0x00, 0xD0, 0x08, 0xE7, 0x3E, 0x00, 0x64}
};

const PROPERTYKEY kDeviceFriendlyNameKey = {
    {0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}},
    14
};
#endif
}

AudioDucker::AudioDucker(QObject *parent) : QObject(parent), pollTimer(new QTimer(this)), rampTimer(new QTimer(this)) {
    currentSettings = loadSettings();
    pollTimer->setInterval(kAudioPollIntervalMs);
    rampTimer->setInterval(kVolumeRampIntervalMs);
    connect(pollTimer, &QTimer::timeout, this, &AudioDucker::pollAudioActivity);
    connect(rampTimer, &QTimer::timeout, this, &AudioDucker::advanceVolumeRamp);
#ifdef _WIN32
    initializeCom();
#endif
    refreshTimerState();
}

AudioDucker::~AudioDucker() {
#ifdef _WIN32
    if (shouldUninitializeCom) {
        CoUninitialize();
    }
#endif
}

AudioDuckerSettings AudioDucker::loadSettings() {
    QSettings settings("SpotifyVol", "SpotifyVol");
    settings.beginGroup(kSettingsGroup);

    AudioDuckerSettings config;
    config.enabled = settings.value("enabled", config.enabled).toBool();
    config.monitorEntireOutput = settings.value("monitorEntireOutput", config.monitorEntireOutput).toBool();
    config.monitorDeviceId = settings.value("monitorDeviceId", config.monitorDeviceId).toString();
    config.monitorDeviceName = settings.value("monitorDeviceName", config.monitorDeviceName).toString();
    config.monitorProcessName = settings.value("monitorProcessName", config.monitorProcessName).toString();
    config.duckedVolume = qBound(0, settings.value("duckedVolume", config.duckedVolume).toInt(), 100);
    config.thresholdPercent = qBound(1, settings.value("thresholdPercent", config.thresholdPercent).toInt(), 100);
    config.cooldownMs = qMax(0, settings.value("cooldownMs", config.cooldownMs).toInt());
    config.releaseHoldMs = qMax(0, settings.value("releaseHoldMs", config.releaseHoldMs).toInt());

    settings.endGroup();
    return config;
}

void AudioDucker::saveSettings(const AudioDuckerSettings &settingsData) {
    QSettings settings("SpotifyVol", "SpotifyVol");
    settings.beginGroup(kSettingsGroup);
    settings.setValue("enabled", settingsData.enabled);
    settings.setValue("monitorEntireOutput", settingsData.monitorEntireOutput);
    settings.setValue("monitorDeviceId", settingsData.monitorDeviceId);
    settings.setValue("monitorDeviceName", settingsData.monitorDeviceName);
    settings.setValue("monitorProcessName", settingsData.monitorProcessName);
    settings.setValue("duckedVolume", settingsData.duckedVolume);
    settings.setValue("thresholdPercent", settingsData.thresholdPercent);
    settings.setValue("cooldownMs", settingsData.cooldownMs);
    settings.setValue("releaseHoldMs", settingsData.releaseHoldMs);
    settings.endGroup();
}

QVector<AudioOutputDeviceOption> AudioDucker::availableOutputDevices() {
    AudioDucker probe;
    return probe.enumerateOutputDevices();
}

void AudioDucker::setSettings(const AudioDuckerSettings &settingsData) {
    currentSettings.enabled = settingsData.enabled;
    currentSettings.monitorEntireOutput = settingsData.monitorEntireOutput;
    currentSettings.monitorDeviceId = settingsData.monitorDeviceId.trimmed();
    currentSettings.monitorDeviceName = settingsData.monitorDeviceName.trimmed();
    currentSettings.monitorProcessName = settingsData.monitorProcessName.trimmed();
    currentSettings.duckedVolume = qBound(0, settingsData.duckedVolume, 100);
    currentSettings.thresholdPercent = qBound(1, settingsData.thresholdPercent, 100);
    currentSettings.cooldownMs = qMax(0, settingsData.cooldownMs);
    currentSettings.releaseHoldMs = qMax(0, settingsData.releaseHoldMs);

    saveSettings(currentSettings);
    refreshTimerState();

    if (!currentSettings.enabled) {
        restoreVolumeIfNeeded();
    }
}

AudioDuckerSettings AudioDucker::settings() const {
    return currentSettings;
}

void AudioDucker::updateSpotifyState(int volume, bool volumeControlSupported) {
    currentSpotifyVolume = qBound(0, volume, 100);
    currentVolumeControlSupported = volumeControlSupported;

    if (pendingRequestedVolume == currentSpotifyVolume) {
        pendingRequestedVolume = -1;
    }

    if (rampTimer->isActive() && currentSpotifyVolume == rampTargetVolume) {
        rampTimer->stop();
        lastRampEmittedVolume = currentSpotifyVolume;
    }

    if (!duckingActive) {
        restoreVolume = currentSpotifyVolume;
    }

    refreshTimerState();
}

void AudioDucker::pollAudioActivity() {
    if (!currentSettings.enabled || !currentVolumeControlSupported || (!currentSettings.monitorEntireOutput && currentSettings.monitorProcessName.isEmpty())) {
        restoreVolumeIfNeeded();
        return;
    }

#ifdef _WIN32
    const bool hasActivity = currentSettings.monitorEntireOutput
        ? (peakLevelForEndpoint() >= (static_cast<double>(currentSettings.thresholdPercent) / 100.0))
        : hasActiveAudioSessionForProcess(currentSettings.monitorProcessName);
    handlePeakLevel(hasActivity ? 1.0 : 0.0);
#else
    updateStatusText("Audio ducking is only implemented on Windows right now.");
#endif
}

void AudioDucker::refreshTimerState() {
    const bool shouldRun = currentSettings.enabled
        && currentVolumeControlSupported
        && (currentSettings.monitorEntireOutput || !currentSettings.monitorProcessName.isEmpty());

    if (shouldRun) {
        if (!pollTimer->isActive()) {
            pollTimer->start();
        }
        updateStatusText(QString("Monitoring %1").arg(currentMonitorLabel()));
    } else {
        pollTimer->stop();
        if (!currentSettings.enabled) {
            updateStatusText("Audio ducking is off.");
        } else if (!currentSettings.monitorEntireOutput && currentSettings.monitorProcessName.isEmpty()) {
            updateStatusText("Choose a monitored app to enable audio ducking.");
        } else if (!currentVolumeControlSupported) {
            updateStatusText("Spotify volume control is unavailable for the current playback device.");
        }
    }
}

void AudioDucker::handlePeakLevel(double peakLevel) {
    const double threshold = static_cast<double>(currentSettings.thresholdPercent) / 100.0;
    const bool detectedActivity = peakLevel >= threshold;

    if (detectedActivity) {
        if (!lastActivityTimer.isValid()) {
            lastActivityTimer.start();
        } else {
            lastActivityTimer.restart();
        }

        if (!duckingActive && currentSpotifyVolume > currentSettings.duckedVolume && canSendVolumeCommand()) {
            restoreVolume = currentSpotifyVolume;
            duckingActive = true;
            requestVolumeChange(currentSettings.duckedVolume);
        }

        updateStatusText(QString("Ducking Spotify while %1 is active").arg(currentMonitorLabel()));
        return;
    }

    if (!duckingActive) {
        updateStatusText(QString("Monitoring %1").arg(currentMonitorLabel()));
        return;
    }

    if (!lastActivityTimer.isValid()) {
        lastActivityTimer.start();
    }

    if (lastActivityTimer.elapsed() >= currentSettings.releaseHoldMs && canSendVolumeCommand()) {
        restoreVolumeIfNeeded();
    } else {
        updateStatusText(QString("Waiting for %1 to stay quiet before restoring volume").arg(currentMonitorLabel()));
    }
}

void AudioDucker::requestVolumeChange(int targetVolume) {
    const int boundedVolume = qBound(0, targetVolume, 100);
    if (boundedVolume == currentSpotifyVolume || pendingRequestedVolume == boundedVolume) {
        return;
    }

    if (!lastVolumeCommandTimer.isValid()) {
        lastVolumeCommandTimer.start();
    } else {
        lastVolumeCommandTimer.restart();
    }

    rampStartVolume = currentSpotifyVolume;
    rampTargetVolume = boundedVolume;
    pendingRequestedVolume = boundedVolume;
    lastRampEmittedVolume = currentSpotifyVolume;

    if (rampStartVolume == rampTargetVolume) {
        pendingRequestedVolume = -1;
        return;
    }

    if (!rampElapsedTimer.isValid()) {
        rampElapsedTimer.start();
    } else {
        rampElapsedTimer.restart();
    }

    if (!rampTimer->isActive()) {
        rampTimer->start();
    }

    advanceVolumeRamp();
}

void AudioDucker::advanceVolumeRamp() {
    if (!currentVolumeControlSupported) {
        rampTimer->stop();
        pendingRequestedVolume = -1;
        return;
    }

    const int elapsedMs = rampElapsedTimer.isValid() ? static_cast<int>(rampElapsedTimer.elapsed()) : kVolumeRampDurationMs;
    const double progress = qBound(0.0, static_cast<double>(elapsedMs) / static_cast<double>(kVolumeRampDurationMs), 1.0);
    const int nextVolume = qBound(
        0,
        qRound(static_cast<double>(rampStartVolume) + static_cast<double>(rampTargetVolume - rampStartVolume) * progress),
        100
    );

    if (nextVolume != lastRampEmittedVolume) {
        lastRampEmittedVolume = nextVolume;
        emit volumeAdjustmentRequested(nextVolume);
    }

    if (progress >= 1.0) {
        rampTimer->stop();
        pendingRequestedVolume = rampTargetVolume;
    }
}

void AudioDucker::restoreVolumeIfNeeded() {
    if (!duckingActive) {
        return;
    }

    duckingActive = false;
    lastActivityTimer.invalidate();

    if (!currentVolumeControlSupported) {
        updateStatusText("Spotify volume control is unavailable for the current playback device.");
        rampTimer->stop();
        pendingRequestedVolume = -1;
        return;
    }

    updateStatusText(QString("Restoring Spotify volume after %1 became quiet").arg(currentMonitorLabel()));
    requestVolumeChange(restoreVolume);
}

bool AudioDucker::canSendVolumeCommand() const {
    return !lastVolumeCommandTimer.isValid() || lastVolumeCommandTimer.elapsed() >= currentSettings.cooldownMs;
}

QString AudioDucker::currentMonitorLabel() const {
    const QString deviceLabel = currentSettings.monitorDeviceName.isEmpty()
        ? QStringLiteral("default output")
        : currentSettings.monitorDeviceName;
    if (currentSettings.monitorEntireOutput) {
        return deviceLabel;
    }
    const QString appLabel = currentSettings.monitorProcessName.isEmpty()
        ? QStringLiteral("the selected app")
        : currentSettings.monitorProcessName;
    return QStringLiteral("%1 on %2").arg(appLabel, deviceLabel);
}

void AudioDucker::updateStatusText(const QString &text) {
    if (lastStatusText == text) {
        return;
    }

    lastStatusText = text;
    emit monitorStateChanged(text);
}

#ifdef _WIN32
bool AudioDucker::initializeCom() {
    const HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(result)) {
        shouldUninitializeCom = true;
        return true;
    }

    if (result == RPC_E_CHANGED_MODE) {
        return true;
    }

    updateStatusText("Audio ducking could not initialize Windows audio APIs.");
    return false;
}

bool AudioDucker::hasActiveAudioSessionForProcess(const QString &processName) const {
    IMMDeviceEnumerator *deviceEnumerator = nullptr;
    IMMDevice *device = nullptr;
    IAudioSessionEnumerator *sessionEnumerator = nullptr;
    bool hasActiveSession = false;

    const HRESULT createResult = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        IID_PPV_ARGS(&deviceEnumerator)
    );
    if (FAILED(createResult) || !deviceEnumerator) {
        return false;
    }

    device = resolveTargetDevice(deviceEnumerator);
    if (!device) {
        deviceEnumerator->Release();
        return false;
    }

    sessionEnumerator = sessionEnumeratorForDevice(device);
    if (!sessionEnumerator) {
        device->Release();
        deviceEnumerator->Release();
        return false;
    }

    int sessionCount = 0;
    sessionEnumerator->GetCount(&sessionCount);
    const QString expectedName = processName.trimmed().toLower();

    for (int index = 0; index < sessionCount; ++index) {
        IAudioSessionControl *sessionControl = nullptr;
        if (FAILED(sessionEnumerator->GetSession(index, &sessionControl)) || !sessionControl) {
            continue;
        }

        IAudioSessionControl2 *sessionControl2 = nullptr;
        if (FAILED(sessionControl->QueryInterface(IID_PPV_ARGS(&sessionControl2))) || !sessionControl2) {
            sessionControl->Release();
            continue;
        }

        AudioSessionState state = AudioSessionStateInactive;
        sessionControl->GetState(&state);
        if (state == AudioSessionStateExpired) {
            sessionControl2->Release();
            sessionControl->Release();
            continue;
        }

        DWORD processId = 0;
        sessionControl2->GetProcessId(&processId);

        if (state == AudioSessionStateActive && processNameForId(processId).toLower() == expectedName) {
            hasActiveSession = true;
            sessionControl2->Release();
            sessionControl->Release();
            break;
        }

        sessionControl2->Release();
        sessionControl->Release();
    }

    sessionEnumerator->Release();
    device->Release();
    deviceEnumerator->Release();
    return hasActiveSession;
}

double AudioDucker::peakLevelForEndpoint() const {
    IMMDeviceEnumerator *deviceEnumerator = nullptr;
    IMMDevice *device = nullptr;
    IAudioMeterInformationLocal *meterInformation = nullptr;
    double peakLevel = 0.0;

    const HRESULT createResult = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        IID_PPV_ARGS(&deviceEnumerator)
    );
    if (FAILED(createResult) || !deviceEnumerator) {
        return false;
    }

    device = resolveTargetDevice(deviceEnumerator);
    if (!device) {
        deviceEnumerator->Release();
        return 0.0;
    }

    if (FAILED(device->Activate(kAudioMeterInformationLocalIid, CLSCTX_ALL, nullptr, reinterpret_cast<void **>(&meterInformation))) || !meterInformation) {
        device->Release();
        deviceEnumerator->Release();
        return 0.0;
    }

    float peakValue = 0.0f;
    if (SUCCEEDED(meterInformation->GetPeakValue(&peakValue))) {
        peakLevel = static_cast<double>(peakValue);
    }

    meterInformation->Release();
    device->Release();
    deviceEnumerator->Release();
    return peakLevel;
}

QVector<AudioOutputDeviceOption> AudioDucker::enumerateOutputDevices() const {
    QVector<AudioOutputDeviceOption> devices;
    IMMDeviceEnumerator *deviceEnumerator = nullptr;
    IMMDeviceCollection *deviceCollection = nullptr;

    const HRESULT createResult = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        IID_PPV_ARGS(&deviceEnumerator)
    );
    if (FAILED(createResult) || !deviceEnumerator) {
        return devices;
    }

    if (FAILED(deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &deviceCollection)) || !deviceCollection) {
        deviceEnumerator->Release();
        return devices;
    }

    UINT deviceCount = 0;
    deviceCollection->GetCount(&deviceCount);
    for (UINT index = 0; index < deviceCount; ++index) {
        IMMDevice *device = nullptr;
        if (FAILED(deviceCollection->Item(index, &device)) || !device) {
            continue;
        }

        AudioOutputDeviceOption option;
        LPWSTR rawId = nullptr;
        if (SUCCEEDED(device->GetId(&rawId)) && rawId) {
            option.id = QString::fromWCharArray(rawId);
            CoTaskMemFree(rawId);
        }
        option.name = friendlyNameForDevice(device);
        if (!option.id.isEmpty() && !option.name.isEmpty()) {
            devices.append(option);
        }
        device->Release();
    }

    deviceCollection->Release();
    deviceEnumerator->Release();
    return devices;
}

IMMDevice *AudioDucker::resolveTargetDevice(IMMDeviceEnumerator *deviceEnumerator) const {
    IMMDevice *device = nullptr;

    if (!currentSettings.monitorDeviceId.isEmpty()) {
        if (SUCCEEDED(deviceEnumerator->GetDevice(reinterpret_cast<LPCWSTR>(currentSettings.monitorDeviceId.utf16()), &device)) && device) {
            return device;
        }
    }

    if (SUCCEEDED(deviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device)) && device) {
        return device;
    }

    return nullptr;
}

IAudioSessionEnumerator *AudioDucker::sessionEnumeratorForDevice(IMMDevice *device) const {
    IAudioSessionManager2 *sessionManager = nullptr;
    IAudioSessionEnumerator *sessionEnumerator = nullptr;

    if (FAILED(device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, reinterpret_cast<void **>(&sessionManager))) || !sessionManager) {
        return nullptr;
    }

    if (FAILED(sessionManager->GetSessionEnumerator(&sessionEnumerator)) || !sessionEnumerator) {
        sessionManager->Release();
        return nullptr;
    }

    sessionManager->Release();
    return sessionEnumerator;
}

QString AudioDucker::friendlyNameForDevice(IMMDevice *device) const {
    IPropertyStore *propertyStore = nullptr;
    PROPVARIANT value;
    PropVariantInit(&value);
    QString result;

    if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &propertyStore)) && propertyStore) {
        if (SUCCEEDED(propertyStore->GetValue(kDeviceFriendlyNameKey, &value)) && value.vt == VT_LPWSTR && value.pwszVal) {
            result = QString::fromWCharArray(value.pwszVal);
        }
        PropVariantClear(&value);
        propertyStore->Release();
    }

    return result;
}

QString AudioDucker::processNameForId(unsigned long processId) const {
    if (processId == 0) {
        return {};
    }

    HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (!processHandle) {
        return {};
    }

    wchar_t pathBuffer[MAX_PATH] = {0};
    DWORD pathLength = MAX_PATH;
    QString result;

    if (QueryFullProcessImageNameW(processHandle, 0, pathBuffer, &pathLength)) {
        result = QString::fromWCharArray(pathBuffer, static_cast<int>(pathLength));
        const int separatorIndex = qMax(result.lastIndexOf('/'), result.lastIndexOf('\\'));
        if (separatorIndex >= 0) {
            result = result.mid(separatorIndex + 1);
        }
    }

    CloseHandle(processHandle);
    return result;
}
#endif

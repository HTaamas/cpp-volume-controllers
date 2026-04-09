#ifndef AUDIO_DUCKER_H
#define AUDIO_DUCKER_H

#include <QObject>
#include <QString>
#include <QElapsedTimer>
#include <QVector>

#ifdef _WIN32
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#endif

struct AudioOutputDeviceOption {
    QString id;
    QString name;
};

struct AudioDuckerSettings {
    bool enabled = false;
    bool monitorEntireOutput = false;
    QString monitorDeviceId;
    QString monitorDeviceName;
    QString monitorProcessName = "Discord.exe";
    int duckedVolume = 25;
    int thresholdPercent = 5;
    int cooldownMs = 500;
    int releaseHoldMs = 1200;
};

class QTimer;

class AudioDucker : public QObject {
    Q_OBJECT
public:
    explicit AudioDucker(QObject *parent = nullptr);
    ~AudioDucker() override;

    static AudioDuckerSettings loadSettings();
    static void saveSettings(const AudioDuckerSettings &settings);
    static QVector<AudioOutputDeviceOption> availableOutputDevices();

    void setSettings(const AudioDuckerSettings &settings);
    AudioDuckerSettings settings() const;
    void updateSpotifyState(int currentVolume, bool volumeControlSupported);

signals:
    void volumeAdjustmentRequested(int targetVolume);
    void monitorStateChanged(const QString &statusText);

private:
    void pollAudioActivity();
    void refreshTimerState();
    void handlePeakLevel(double peakLevel);
    void requestVolumeChange(int targetVolume);
    void advanceVolumeRamp();
    void restoreVolumeIfNeeded();
    bool canSendVolumeCommand() const;
    QString currentMonitorLabel() const;
    void updateStatusText(const QString &text);

#ifdef _WIN32
    bool hasActiveAudioSessionForProcess(const QString &processName) const;
    double peakLevelForEndpoint() const;
    QVector<AudioOutputDeviceOption> enumerateOutputDevices() const;
    IMMDevice *resolveTargetDevice(IMMDeviceEnumerator *deviceEnumerator) const;
    IAudioSessionEnumerator *sessionEnumeratorForDevice(IMMDevice *device) const;
    QString friendlyNameForDevice(IMMDevice *device) const;
    QString processNameForId(unsigned long processId) const;
    bool initializeCom();
#endif

    AudioDuckerSettings currentSettings;
    QTimer *pollTimer;
    QTimer *rampTimer;
    bool currentVolumeControlSupported = true;
    int currentSpotifyVolume = 50;
    int restoreVolume = 50;
    bool duckingActive = false;
    int pendingRequestedVolume = -1;
    int rampStartVolume = 50;
    int rampTargetVolume = 50;
    int lastRampEmittedVolume = -1;
    QString lastStatusText;
    QElapsedTimer lastActivityTimer;
    QElapsedTimer lastVolumeCommandTimer;
    QElapsedTimer rampElapsedTimer;

#ifdef _WIN32
    bool shouldUninitializeCom = false;
#endif
};

#endif // AUDIO_DUCKER_H

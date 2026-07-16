#include "app_settings.h"
#include <QRandomGenerator>

namespace {
constexpr auto kOverlayGroup = "overlay";
constexpr auto kKeybindsGroup = "keybinds";
}

namespace AppSettings {
OverlaySettings loadOverlaySettings() {
    QSettings settings("SpotifyVol", "SpotifyVol");
    settings.beginGroup(kOverlayGroup);

    OverlaySettings config;
    config.backgroundColor = settings.value("backgroundColor", config.backgroundColor).toString();
    config.borderColor = settings.value("borderColor", config.borderColor).toString();
    config.accentColor = settings.value("accentColor", config.accentColor).toString();
    config.primaryTextColor = settings.value("primaryTextColor", config.primaryTextColor).toString();
    config.secondaryTextColor = settings.value("secondaryTextColor", config.secondaryTextColor).toString();
    config.mutedTextColor = settings.value("mutedTextColor", config.mutedTextColor).toString();
    config.progressBarColor = settings.value("progressBarColor", config.progressBarColor).toString();
    config.overlayWidth = qMax(320, settings.value("overlayWidth", config.overlayWidth).toInt());
    config.hideDurationMs = qMax(1000, settings.value("hideDurationMs", config.hideDurationMs).toInt());

    settings.endGroup();
    return config;
}

void saveOverlaySettings(const OverlaySettings &config) {
    QSettings settings("SpotifyVol", "SpotifyVol");
    settings.beginGroup(kOverlayGroup);
    settings.setValue("backgroundColor", config.backgroundColor);
    settings.setValue("borderColor", config.borderColor);
    settings.setValue("accentColor", config.accentColor);
    settings.setValue("primaryTextColor", config.primaryTextColor);
    settings.setValue("secondaryTextColor", config.secondaryTextColor);
    settings.setValue("mutedTextColor", config.mutedTextColor);
    settings.setValue("progressBarColor", config.progressBarColor);
    settings.setValue("overlayWidth", config.overlayWidth);
    settings.setValue("hideDurationMs", config.hideDurationMs);
    settings.endGroup();
}

QString loadRefreshToken() {
    QSettings settings("SpotifyVol", "SpotifyVol");
    return settings.value("RefreshToken").toString();
}

void saveRefreshToken(const QString &token) {
    QSettings settings("SpotifyVol", "SpotifyVol");
    settings.setValue("RefreshToken", token);
}

void clearRefreshToken() {
    QSettings settings("SpotifyVol", "SpotifyVol");
    settings.remove("RefreshToken");
}

QString loadOrCreateDeviceId() {
    QSettings settings("SpotifyVol", "SpotifyVol");
    QString deviceId = settings.value("DeviceId").toString();
    if (deviceId.length() != 40) {
        deviceId.clear();
        for (int i = 0; i < 20; ++i) {
            deviceId += QString("%1").arg(QRandomGenerator::global()->bounded(256), 2, 16, QChar('0'));
        }
        settings.setValue("DeviceId", deviceId);
    }
    return deviceId;
}

KeybindSettings loadKeybindSettings() {
    QSettings settings("SpotifyVol", "SpotifyVol");
    settings.beginGroup(kKeybindsGroup);

    KeybindSettings config;
    config.coarseStep = qBound(1, settings.value("coarseStep", config.coarseStep).toInt(), 25);
    config.fineStep = qBound(1, settings.value("fineStep", config.fineStep).toInt(), 25);
    config.useShiftForFineAdjust = settings.value("useShiftForFineAdjust", config.useShiftForFineAdjust).toBool();
    config.mainKey = settings.value("mainKey", config.mainKey).toString();

    settings.endGroup();
    return config;
}

void saveKeybindSettings(const KeybindSettings &config) {
    QSettings settings("SpotifyVol", "SpotifyVol");
    settings.beginGroup(kKeybindsGroup);
    settings.setValue("coarseStep", config.coarseStep);
    settings.setValue("fineStep", config.fineStep);
    settings.setValue("useShiftForFineAdjust", config.useShiftForFineAdjust);
    settings.setValue("mainKey", config.mainKey);
    settings.endGroup();
}
}

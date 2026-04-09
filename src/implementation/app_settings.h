#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <QSettings>
#include <QString>

struct OverlaySettings {
    QString backgroundColor = "#1c1c1c";
    QString borderColor = "#333333";
    QString accentColor = "#1DB954";
    QString primaryTextColor = "#ffffff";
    QString secondaryTextColor = "#aaaaaa";
    QString mutedTextColor = "#888888";
    QString progressBarColor = "#ffffff";
    int overlayWidth = 440;
    int hideDurationMs = 3500;
};

struct KeybindSettings {
    int coarseStep = 5;
    int fineStep = 1;
    bool useShiftForFineAdjust = true;
};

namespace AppSettings {
OverlaySettings loadOverlaySettings();
void saveOverlaySettings(const OverlaySettings &settings);

KeybindSettings loadKeybindSettings();
void saveKeybindSettings(const KeybindSettings &settings);
}

#endif // APP_SETTINGS_H

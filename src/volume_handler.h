#ifndef VOLUME_HANDLER_H
#define VOLUME_HANDLER_H

#include <QObject>
#include "implementation/app_settings.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __linux__
#include <X11/Xlib.h>
#endif

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#endif

class QSocketNotifier;

class VolumeHandler : public QObject {
    Q_OBJECT
public:
    explicit VolumeHandler(QObject *parent = nullptr);
    ~VolumeHandler() override;
    void applyKeybindSettings(const KeybindSettings &settings);

signals:
    void volumeChanged(int delta);
    void toggleDuckingRequested();

private:
#ifdef _WIN32
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static HHOOK hHook;
    static bool duckingToggleChordDown;
#endif
#ifdef __APPLE__
    static CGEventRef MacEventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);
    CFMachPortRef eventTap = nullptr;
    CFRunLoopSourceRef runLoopSource = nullptr;
#endif
#ifdef __linux__
    void processLinuxX11Events();
    void releaseLinuxGrabs();
    Display *x11Display = nullptr;
    Window x11RootWindow = 0;
    QSocketNotifier *x11Notifier = nullptr;
    int volumeUpKeyCode = 0;
    int volumeDownKeyCode = 0;
#endif
    static VolumeHandler *instance;
    KeybindSettings keybindSettings;
};

#endif // VOLUME_HANDLER_H

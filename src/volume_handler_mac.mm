#include "volume_handler.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QMessageBox>

#ifdef __APPLE__
#import <AppKit/AppKit.h>
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <IOKit/hidsystem/IOLLEvent.h>
#include <IOKit/hidsystem/ev_keymap.h>

namespace {
constexpr int kKeyStateDown = 0x0A;

bool isVolumeKey(int keyCode) {
    return keyCode == NX_KEYTYPE_SOUND_UP || keyCode == NX_KEYTYPE_SOUND_DOWN;
}

int mainKeyCodeFromSettings(const KeybindSettings &settings) {
    bool ok = false;
    const int vkCode = settings.mainKey.toInt(&ok, 16);
    if (!ok) {
        return -1;
    }

    switch (vkCode) {
    case 0x08:
        return kVK_Delete;
    case 0x09:
        return kVK_Tab;
    case 0x0D:
        return kVK_Return;
    case 0x10:
        return kVK_Shift;
    case 0x11:
        return kVK_Control;
    case 0x12:
        return kVK_Option;
    case 0x13:
        return kVK_Command;
    case 0x14:
        return kVK_CapsLock;
    case 0x1B:
        return kVK_Escape;
    case 0x20:
        return kVK_Space;
    case 0x21:
        return kVK_PageUp;
    case 0x22:
        return kVK_PageDown;
    case 0x23:
        return kVK_End;
    case 0x24:
        return kVK_Home;
    case 0x25:
        return kVK_LeftArrow;
    case 0x26:
        return kVK_UpArrow;
    case 0x27:
        return kVK_RightArrow;
    case 0x28:
        return kVK_DownArrow;
    case 0x2E:
        return kVK_ForwardDelete;
    default:
        break;
    }

    if (vkCode >= 0x30 && vkCode <= 0x39) {
        static const int digitMap[] = {
            kVK_ANSI_0,
            kVK_ANSI_1,
            kVK_ANSI_2,
            kVK_ANSI_3,
            kVK_ANSI_4,
            kVK_ANSI_5,
            kVK_ANSI_6,
            kVK_ANSI_7,
            kVK_ANSI_8,
            kVK_ANSI_9,
        };
        return digitMap[vkCode - 0x30];
    }

    if (vkCode >= 0x41 && vkCode <= 0x5A) {
        return kVK_ANSI_A + (vkCode - 0x41);
    }

    if (vkCode >= 0x70 && vkCode <= 0x7B) {
        return kVK_F1 + (vkCode - 0x70);
    }

    return vkCode;
}

bool ensureMacInputAccess() {
    bool hasListenAccess = true;
    if (@available(macOS 10.15, *)) {
        hasListenAccess = CGPreflightListenEventAccess();
        if (!hasListenAccess) {
            CGRequestListenEventAccess();
            hasListenAccess = CGPreflightListenEventAccess();
        }
    }

    const NSDictionary *options = @{
        (__bridge NSString *)kAXTrustedCheckOptionPrompt: @YES
    };
    const bool hasAccessibilityAccess = AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);

    return hasListenAccess && hasAccessibilityAccess;
}
}

VolumeHandler::VolumeHandler(QObject *parent) : QObject(parent) {
    instance = this;

    if (!ensureMacInputAccess()) {
        QMessageBox::information(
            nullptr,
            "SpotifyVol Permission Needed",
            "To intercept the volume keys as a bundled app, allow SpotifyVol in System Settings under Privacy & Security > Input Monitoring and Accessibility, then reopen the app."
        );
        qDebug() << "macOS global volume interception requires Input Monitoring and Accessibility access.";
        return;
    }

    eventTap = CGEventTapCreate(
        kCGSessionEventTap,
        kCGHeadInsertEventTap,
        kCGEventTapOptionDefault,
        CGEventMaskBit(NX_SYSDEFINED) | CGEventMaskBit(kCGEventKeyDown),
        MacEventTapCallback,
        this
    );

    if (!eventTap) {
        qDebug() << "Failed to install macOS event tap. Input Monitoring permission may be required.";
        return;
    }

    runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    if (!runLoopSource) {
        qDebug() << "Failed to create macOS event tap run loop source.";
        CFRelease(eventTap);
        eventTap = nullptr;
        return;
    }

    CFRunLoopAddSource(CFRunLoopGetMain(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);
}

VolumeHandler::~VolumeHandler() {
    if (runLoopSource) {
        CFRunLoopRemoveSource(CFRunLoopGetMain(), runLoopSource, kCFRunLoopCommonModes);
        CFRelease(runLoopSource);
        runLoopSource = nullptr;
    }

    if (eventTap) {
        CFMachPortInvalidate(eventTap);
        CFRelease(eventTap);
        eventTap = nullptr;
    }

    if (instance == this) {
        instance = nullptr;
    }
}

CGEventRef VolumeHandler::MacEventTapCallback(CGEventTapProxy, CGEventType type, CGEventRef event, void *refcon) {
    auto *handler = static_cast<VolumeHandler *>(refcon);
    if (!handler) {
        return event;
    }

    static QElapsedTimer nextTrackTimer;
    static QElapsedTimer prevTrackTimer;
    const int doubleTapTimeout = 400;

    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
        if (handler->eventTap) {
            CGEventTapEnable(handler->eventTap, true);
        }
        return event;
    }

    if (type == kCGEventFlagsChanged || type == kCGEventKeyDown) {
        const int configuredMainKeyCode = mainKeyCodeFromSettings(handler->keybindSettings);
        if (configuredMainKeyCode < 0) {
            return event;
        }

        const int keyCode = static_cast<int>(CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));
        if (keyCode != configuredMainKeyCode) {
            return event;
        }

        if (type == kCGEventFlagsChanged && keyCode != kVK_CapsLock) {
            return event;
        }

        const CGEventFlags flags = CGEventGetFlags(event);
        const bool isShift = (flags & kCGEventFlagMaskShift) != 0;
        const bool isCtrl = (flags & kCGEventFlagMaskControl) != 0;

        if (isShift && isCtrl) {
            return nullptr;
        }

        if (isShift) {
            if (!nextTrackTimer.isValid() || nextTrackTimer.elapsed() > doubleTapTimeout) {
                nextTrackTimer.start();
            } else {
                nextTrackTimer.invalidate();
                if (instance) {
                    emit instance->nextTrack();
                }
            }
        } else if (isCtrl) {
            if (!prevTrackTimer.isValid() || prevTrackTimer.elapsed() > doubleTapTimeout) {
                prevTrackTimer.start();
            } else {
                prevTrackTimer.invalidate();
                if (instance) {
                    emit instance->prevTrack();
                }
            }
        } else if (instance) {
            emit instance->toggleMusic();
        }

        return nullptr;
    }

    if (type != static_cast<CGEventType>(NX_SYSDEFINED)) {
        return event;
    }

    NSEvent *nsEvent = [NSEvent eventWithCGEvent:event];
    if (!nsEvent || nsEvent.subtype != NX_SUBTYPE_AUX_CONTROL_BUTTONS) {
        return event;
    }

    const NSInteger data = nsEvent.data1;
    const int keyCode = static_cast<int>((data & 0xFFFF0000) >> 16);
    const int keyState = static_cast<int>((data & 0x0000FF00) >> 8);

    if (!isVolumeKey(keyCode)) {
        return event;
    }

    if (keyState == kKeyStateDown && instance) {
        const bool isShift = (CGEventGetFlags(event) & kCGEventFlagMaskShift) != 0;
        const int delta = (keyCode == NX_KEYTYPE_SOUND_UP) ? (isShift ? 1 : 5) : (isShift ? -1 : -5);
        emit instance->volumeChanged(delta);
    }

    return nullptr;
}
#endif

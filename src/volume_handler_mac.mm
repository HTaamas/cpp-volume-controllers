#include "volume_handler.h"
#include <QDebug>
#include <QMessageBox>
#include <QMetaObject>

#ifdef __APPLE__
#import <AppKit/AppKit.h>
#include <IOKit/hidsystem/ev_keymap.h>
#include <IOKit/hidsystem/IOLLEvent.h>
#include <ApplicationServices/ApplicationServices.h>

namespace {
constexpr int kKeyStateDown = 0x0A;

bool isVolumeKey(int keyCode) {
    return keyCode == NX_KEYTYPE_SOUND_UP || keyCode == NX_KEYTYPE_SOUND_DOWN;
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
        CGEventMaskBit(NX_SYSDEFINED),
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

    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
        if (handler->eventTap) {
            CGEventTapEnable(handler->eventTap, true);
        }
        return event;
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
        const bool isShift = (nsEvent.modifierFlags & NSEventModifierFlagShift) != 0;
        const int delta = (keyCode == NX_KEYTYPE_SOUND_UP) ? (isShift ? 1 : 5) : (isShift ? -1 : -5);
        QMetaObject::invokeMethod(instance, [delta]() {
            if (instance) {
                emit instance->volumeChanged(delta);
            }
        }, Qt::QueuedConnection);
    }

    return nullptr;
}
#endif

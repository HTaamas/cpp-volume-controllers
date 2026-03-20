#import <AppKit/AppKit.h>
#include "volume_handler_mac.h"
#include <IOKit/hidsystem/ev_keymap.h>
#include <QDebug>
#include <ApplicationServices/ApplicationServices.h>

VolumeHandlerMac::VolumeHandlerMac(QObject *parent) : VolumeHandler(parent) {
    instance = this;
    
    // Hide the app from the Dock
    [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

    // Use kCGHIDEventTap to intercept at the lowest level. 
    // This is required for many media keys on modern macOS.
    CGEventMask mask = (CGEventMask)1 << 14; // kCGEventSystemDefined
    eventTap = CGEventTapCreate(kCGHIDEventTap, 
                                kCGHeadInsertEventTap, 
                                kCGEventTapOptionDefault, 
                                mask, 
                                eventCallback, 
                                this);
    
    if (!eventTap) {
        qDebug() << "CRITICAL: Failed to create event tap! Make sure the app has Accessibility permissions in System Settings.";
        return;
    }

    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);
    CFRelease(runLoopSource);
}

VolumeHandlerMac::~VolumeHandlerMac() {
    if (eventTap) {
        CFRelease(eventTap);
    }
}

CGEventRef VolumeHandlerMac::eventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon) {
    VolumeHandlerMac *self = static_cast<VolumeHandlerMac*>(refcon);
    
    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
        if (self && self->eventTap) {
            CGEventTapEnable(self->eventTap, true);
        }
        return event;
    }

    // 14 is the value for kCGEventSystemDefined
    if ((int)type != 14) return event;

    @autoreleasepool {
        // Access the raw event data directly for better reliability
        NSEvent *nsEvent = [NSEvent eventWithCGEvent:event];
        if (nsEvent.type == NSEventTypeSystemDefined && nsEvent.subtype == 8) {
            int data = (int)nsEvent.data1;
            int keyCode = ((data & 0xFFFF0000) >> 16);
            int keyFlags = (data & 0x0000FFFF);
            int keyState = (((keyFlags & 0xFF00) >> 8)) == 0xA; // 0xA is down, 0xB is up
            
            if (keyCode == NX_KEYTYPE_SOUND_UP || keyCode == NX_KEYTYPE_SOUND_DOWN) {
                if (keyState) {
                    bool isShift = (nsEvent.modifierFlags & NSEventModifierFlagShift);
                    int delta = (keyCode == NX_KEYTYPE_SOUND_UP) ? (isShift ? 1 : 5) : (isShift ? -1 : -5);
                    emit instance->volumeChanged(delta);
                }
                return NULL; // Suppress the event from reaching the system
            }
        }
    }
    
    return event;
}

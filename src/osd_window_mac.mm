#include <QWidget>

#ifdef __APPLE__
#import <AppKit/AppKit.h>

void applyMacOverlayWindowBehavior(QWidget *widget) {
    if (!widget) {
        return;
    }

    widget->winId();

    NSView *view = reinterpret_cast<NSView *>(widget->winId());
    if (!view) {
        return;
    }

    NSWindow *window = view.window;
    if (!window) {
        return;
    }

    const NSWindowStyleMask styleMask =
        ([window styleMask] | NSWindowStyleMaskNonactivatingPanel) & ~NSWindowStyleMaskTitled;

    [window setStyleMask:styleMask];

    const NSWindowCollectionBehavior behavior =
        NSWindowCollectionBehaviorMoveToActiveSpace |
        NSWindowCollectionBehaviorFullScreenAuxiliary |
        NSWindowCollectionBehaviorIgnoresCycle |
        NSWindowCollectionBehaviorTransient;

    [window setCollectionBehavior:behavior];
    [window setLevel:NSFloatingWindowLevel];
    [window setHidesOnDeactivate:NO];
    [window setReleasedWhenClosed:NO];
    [window setIgnoresMouseEvents:YES];
    [window setExcludedFromWindowsMenu:YES];
    [window setAnimationBehavior:NSWindowAnimationBehaviorUtilityWindow];
    [window setMovable:NO];
    [window setMovableByWindowBackground:NO];
    [window setCanHide:NO];
    if ([window respondsToSelector:@selector(setBecomesKeyOnlyIfNeeded:)]) {
        [(id)window setBecomesKeyOnlyIfNeeded:YES];
    }
    [window orderFrontRegardless];
}
#endif

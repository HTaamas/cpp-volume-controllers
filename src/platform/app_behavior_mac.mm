#ifdef __APPLE__
#import <AppKit/AppKit.h>

void configureMacApplicationBehavior() {
    NSApplication *app = [NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyAccessory];
}
#endif

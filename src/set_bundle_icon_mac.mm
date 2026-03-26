#ifdef __APPLE__
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

int main(int argc, char *argv[]) {
    @autoreleasepool {
        if (argc != 3) {
            return 1;
        }

        NSString *bundlePath = [NSString stringWithUTF8String:argv[1]];
        NSString *iconPath = [NSString stringWithUTF8String:argv[2]];
        NSImage *icon = [[NSImage alloc] initWithContentsOfFile:iconPath];
        if (!icon) {
            return 2;
        }

        return [[NSWorkspace sharedWorkspace] setIcon:icon forFile:bundlePath options:0] ? 0 : 3;
    }
}
#endif

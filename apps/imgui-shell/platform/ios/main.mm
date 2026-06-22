// main.mm (iOS host)
// Entry point for the iOS app: hands control to UIKit via UIApplicationMain,
// which owns the run loop and instantiates AppDelegate. Unlike the desktop host,
// the app does not drive its own frame loop here — see ViewController.mm.
#import <UIKit/UIKit.h>
#import "AppDelegate.h"

int main(int argc, char* argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}

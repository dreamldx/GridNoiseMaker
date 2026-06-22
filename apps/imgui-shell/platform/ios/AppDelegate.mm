// AppDelegate.mm (iOS host)
// Minimal UIApplicationDelegate: on launch it creates the key UIWindow and
// installs ViewController (which owns Metal + ImGui) as its root. All rendering
// and lifecycle work lives in ViewController.mm.
#import "AppDelegate.h"
#import "ViewController.h"

@implementation AppDelegate

- (BOOL)application:(UIApplication*)application
    didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
    (void)application;
    (void)launchOptions;
    self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
    self.window.rootViewController = [[ViewController alloc] init];
    [self.window makeKeyAndVisible];
    return YES;
}

@end

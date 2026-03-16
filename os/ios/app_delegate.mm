// LAF OS Library
// Copyright (c) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#import <UIKit/UIKit.h>
#include <thread>

extern int app_main(int argc, char* argv[]);

static int g_argc = 0;
static char** g_argv = nullptr;

@interface AsepriteAppDelegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow* window;
@end

@implementation AsepriteAppDelegate

- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
  // Launch app_main in a separate thread so UIKit run loop can proceed
  std::thread appThread([]() {
    @autoreleasepool {
      app_main(g_argc, g_argv);
    }
  });
  appThread.detach();
  return YES;
}

@end

namespace os {

int ios_main(int argc, char* argv[])
{
  g_argc = argc;
  g_argv = argv;
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil,
                             NSStringFromClass([AsepriteAppDelegate class]));
  }
}

} // namespace os

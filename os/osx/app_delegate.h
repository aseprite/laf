// LAF OS Library
// Copyright (C) 2020-present  Igara Studio S.A.
// Copyright (C) 2012-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_OSX_APP_DELEGATE_H_INCLUDED
#define OS_OSX_APP_DELEGATE_H_INCLUDED
#pragma once

#include <Cocoa/Cocoa.h>

#include <set>
#include <string>

@interface NSApplicationOSX : NSApplication
- (void)sendEvent:(NSEvent*)event;
@end

@interface AppDelegateOSX : NSObject <NSApplicationDelegate> {
  // Files that were already processed in the CLI, so we don't need to
  // generate a DropFiles event.
  std::set<std::string> m_cliFiles;
  // When a main window contains child windows, the standard macOS
  // multiple-window behavior is insufficient to emulate the same
  // behavior as Windows.
  // On Windows, child windows preserve their z-order based on the
  // last focused window, except for modal parent windows, which stay
  // behind their children. Additionally, parent window positioning is
  // independent from its child windows.
  // The window z-order must also remain intact when the application is
  // deactivated and activated again. However, macOS rearranges child
  // windows on reactivation.
  // Because of this, we maintain the following vector to store the
  // window z-order whenever focus changes, allowing us to restore the
  // same order when the application becomes active again.
  NSArray<NSWindow*>* __strong m_savedWindowOrder;
  bool m_isHidden;
}
- (id)init;
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender;
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)app;
- (void)applicationWillTerminate:(NSNotification*)notification;
- (void)applicationDidResignActive:(NSNotification*)notification;
- (void)applicationWillResignActive:(NSNotification*)notification;
- (void)applicationDidFinishLaunching:(NSNotification*)notification;
- (void)applicationWillBecomeActive:(NSNotification*)notification;
- (void)applicationDidBecomeActive:(NSNotification*)notification;
- (void)applicationDidHide:(NSNotification*)notification;
- (void)applicationDidUnhide:(NSNotification*)notification;
- (BOOL)application:(NSApplication*)app openFiles:(NSArray*)filenames;
- (void)executeMenuItem:(id)sender;
- (BOOL)validateMenuItem:(NSMenuItem*)menuItem;

- (void)markCliFileAsProcessed:(const std::string&)fn;
- (void)resetCliFiles;
- (BOOL)isHidden;
@end

#endif

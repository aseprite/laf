// LAF OS Library
// Copyright (C) 2020-2024  Igara Studio S.A.
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
  bool m_isHidden;
  NSMutableArray<NSWindow*>* m_floatingWindows;
}

- (id)init;
- (NSApplicationTerminateReply)applicationShouldTerminate:
  (NSApplication*)sender;
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)app;
- (void)applicationWillTerminate:(NSNotification*)notification;
- (void)applicationWillResignActive:(NSNotification*)notification;
- (void)applicationDidBecomeActive:(NSNotification*)notification;
- (void)applicationDidHide:(NSNotification*)notification;
- (void)applicationDidUnhide:(NSNotification*)notification;
- (BOOL)application:(NSApplication*)app openFiles:(NSArray*)filenames;
- (void)executeMenuItem:(id)sender;
- (BOOL)validateMenuItem:(NSMenuItem*)menuItem;

- (void)markCliFileAsProcessed:(const std::string&)fn;
- (void)resetCliFiles;
- (BOOL)isHidden;
- (NSMutableArray<NSWindow*>*)floatingWindows;
@end

#endif

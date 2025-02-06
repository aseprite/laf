// LAF OS Library
// Copyright (C) 2020-2024  Igara Studio S.A.
// Copyright (C) 2012-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <Cocoa/Cocoa.h>

#include "os/osx/app_delegate.h"
#include "os/osx/window.h"

#include "base/fs.h"
#include "os/event.h"
#include "os/event_queue.h"
#include "os/osx/app.h"
#include "os/osx/view.h"
#include "os/system.h"

@implementation NSApplicationOSX

- (void)sendEvent:(NSEvent*)event
{
  if ([event type] == NSKeyDown) {
    if (([event modifierFlags] & NSDeviceIndependentModifierFlagsMask) == NSCommandKeyMask) {
      if ([[event charactersIgnoringModifiers] isEqualToString:@"x"]) {
        if ([self sendAction:@selector(cut:) to:nil from:self])
          return;
      }
      else if ([[event charactersIgnoringModifiers] isEqualToString:@"c"]) {
        if ([self sendAction:@selector(copy:) to:nil from:self])
          return;
      }
      else if ([[event charactersIgnoringModifiers] isEqualToString:@"v"]) {
        if ([self sendAction:@selector(paste:) to:nil from:self])
          return;
      }
      else if ([[event charactersIgnoringModifiers] isEqualToString:@"z"]) {
        if ([self sendAction:@selector(undo:) to:nil from:self])
          return;
      }
      else if ([[event charactersIgnoringModifiers] isEqualToString:@"a"]) {
        if ([self sendAction:@selector(selectAll:) to:nil from:self])
          return;
      }
    }
    else if (([event modifierFlags] & NSDeviceIndependentModifierFlagsMask) ==
             (NSCommandKeyMask | NSShiftKeyMask)) {
      if ([[event charactersIgnoringModifiers] isEqualToString:@"Z"]) {
        if ([self sendAction:@selector(redo:) to:nil from:self])
          return;
      }
    }
  }
  [super sendEvent:event];
}

@end

@protocol ValidateMenuItemProtocolOSX
- (void)validateLafMenuItem;
@end

@implementation AppDelegateOSX

- (id)init
{
  if (self = [super init]) {
    m_isHidden = false;
  }
  return self;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
  os::Event ev;
  ev.setType(os::Event::CloseApp);
  os::queue_event(ev);
  return NSTerminateCancel;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)app
{
  return YES;
}

- (void)applicationWillTerminate:(NSNotification*)notification
{
}

- (void)applicationWillResignActive:(NSNotification*)notification
{
  for (id window : [NSApp windows]) {
    if ([window isKindOfClass:[WindowOSXObjc class]] && [window isFloating]) {
      [window setLevel:NSNormalWindowLevel];
      [window orderWindow:NSWindowAbove relativeTo:0];
    }
  }

  NSEvent* event = [NSApp currentEvent];
  if (event != nil)
    [ViewOSX updateKeyFlags:event];
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
  // We do not want that the [NSApplication run] call made from
  // AppOSX::Impl::finishLaunching() blocks.
  [NSApp stop:nil];
}

- (void)applicationDidBecomeActive:(NSNotification*)notification
{
  for (id window : [NSApp windows]) {
    if ([window isKindOfClass:[WindowOSXObjc class]] && [window isFloating]) {
      [window setLevel:NSFloatingWindowLevel];
    }
  }

  NSEvent* event = [NSApp currentEvent];
  if (event != nil)
    [ViewOSX updateKeyFlags:event];
}

- (void)applicationDidHide:(NSNotification*)notification
{
  m_isHidden = true;
}

- (void)applicationDidUnhide:(NSNotification*)notification
{
  m_isHidden = false;
}

- (BOOL)application:(NSApplication*)app openFiles:(NSArray*)filenames
{
  // TODO similar to generate_drop_files_from_nsarray() but with a
  // filter for files that were already processed in the CLI (m_cliFiles)

  base::paths files;
  for (int i = 0; i < [filenames count]; ++i) {
    NSString* fnString = [filenames objectAtIndex:i];
    std::string fn = base::normalize_path([fnString UTF8String]);
    if (m_cliFiles.find(fn) == m_cliFiles.end())
      files.push_back(fn);
  }

  if (!files.empty()) {
    os::Event ev;
    ev.setType(os::Event::DropFiles);
    ev.setFiles(files);
    os::queue_event(ev);
  }

  [app replyToOpenOrPrint:NSApplicationDelegateReplySuccess];
  return YES;
}

- (void)executeMenuItem:(id)sender
{
  [sender executeMenuItem:sender];
}

- (BOOL)validateMenuItem:(NSMenuItem*)menuItem
{
  if (menuItem && [menuItem respondsToSelector:@selector(validateLafMenuItem)]) {
    [((id<ValidateMenuItemProtocolOSX>)menuItem) validateLafMenuItem];
    return menuItem.enabled;
  }
  else
    return [super validateMenuItem:menuItem];
}

- (void)markCliFileAsProcessed:(const std::string&)fn
{
  m_cliFiles.insert(fn);
}

- (void)resetCliFiles
{
  // After finishLaunching() we clear the filter
  m_cliFiles.clear();
}

- (BOOL)isHidden
{
  return m_isHidden;
}

@end

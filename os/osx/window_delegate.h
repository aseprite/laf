// LAF OS Library
// Copyright (C) 2020-2024  Igara Studio S.A.
// Copyright (C) 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

namespace os {
  class WindowOSX;
}

@interface WindowOSXDelegate : NSObject {
@private
  os::WindowOSX* __weak m_impl;
  NSRect m_maximizedFrame;
}
@end

@implementation WindowOSXDelegate

- (WindowOSXDelegate*)initWithWindowImpl:(os::WindowOSX*)impl
{
  m_impl = impl;
  return self;
}

- (void)removeImpl
{
  m_impl = nullptr;
}

- (BOOL)windowShouldClose:(NSWindow*)sender
{
  if (m_impl) {
    os::Event ev;
    ev.setType(os::Event::CloseWindow);
    m_impl->queueEvent(ev);
  }
  return NO;
}

- (void)windowWillClose:(NSNotification*)notification
{
  if (m_impl)
    m_impl->onClose();
}

- (void)windowWillStartLiveResize:(NSNotification*)notification
{
  if (m_impl)
    m_impl->onStartResizing();
}

- (void)windowDidResize:(NSNotification*)notification
{
  NSView* view = [notification.object contentView];
  gfx::Size sz(view.bounds.size.width, view.bounds.size.height);
  if (m_impl)
    m_impl->onResizing(sz);
}

- (void)windowDidEndLiveResize:(NSNotification*)notification
{
  if (m_impl)
    m_impl->onEndResizing();
}

- (void)windowDidMiniaturize:(NSNotification*)notification
{
}

- (void)windowWillEnterFullScreen:(NSNotification*)notification
{
  if (m_impl)
    m_impl->onStartResizing();
}

- (void)windowDidEnterFullScreen:(NSNotification*)notification
{
  if (m_impl)
    m_impl->onEndResizing();
}

- (void)windowWillExitFullScreen:(NSNotification*)notification
{
  if (m_impl)
    m_impl->onStartResizing();
}

- (void)windowDidExitFullScreen:(NSNotification*)notification
{
  // After exiting the full screen mode we have to re-create the skia
  // surface and re-draw the entire screen. Without this there will be
  // some cases where the app view is not updated anymore until we
  // resize the window.
  if (m_impl)
    m_impl->onEndResizing();
}

- (NSRect)windowWillUseStandardFrame:(NSWindow*)window
                        defaultFrame:(NSRect)newFrame
{
  // In "newFrame" is the frame size when a window is maximized in the
  // current screen. We can cache this value to know when we are
  // maximized after a zoom command.
  m_maximizedFrame = newFrame;
  return newFrame;
}

- (BOOL)windowShouldZoom:(NSWindow*)window
                 toFrame:(NSRect)newFrame
{
  if (m_impl) {
    if (NSEqualRects(newFrame, m_maximizedFrame))
      m_impl->onBeforeMaximizeFrame();
  }
  return YES;
}

- (BOOL)windowShouldBecomeKey:(NSWindow *)sender
{
  NSWindow* modalWindow = [NSApp modalWindow];
  return !modalWindow || modalWindow == sender;
}

@end

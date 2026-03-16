// LAF OS Library
// Copyright (c) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "os/ios/window.h"
#include "os/ios/view.h"

#import <UIKit/UIKit.h>

// Custom VC that defers system edge gestures so touches reach our view
@interface AsepriteViewController : UIViewController
@end

@implementation AsepriteViewController

- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures
{
  return UIRectEdgeAll;
}

- (BOOL)prefersStatusBarHidden
{
  return YES;
}

- (BOOL)prefersHomeIndicatorAutoHidden
{
  return YES;
}

@end

namespace os {

void WindowIOS::createWindow(const os::WindowSpec& spec)
{
  __block bool done = false;

  auto createBlock = ^{
    @autoreleasepool {
      UIWindowScene* scene = nil;
      for (UIScene* s in UIApplication.sharedApplication.connectedScenes) {
        if ([s isKindOfClass:[UIWindowScene class]]) {
          scene = (UIWindowScene*)s;
          break;
        }
      }

      if (scene) {
        m_uiWindow = [[UIWindow alloc] initWithWindowScene:scene];
      }
      else {
        m_uiWindow = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
      }

      AsepriteViewController* vc = [[AsepriteViewController alloc] init];
      // Use a container view with black background for safe area
      UIView* container = [[UIView alloc] initWithFrame:UIScreen.mainScreen.bounds];
      container.backgroundColor = UIColor.blackColor;
      container.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
      vc.view = container;
      m_uiWindow.rootViewController = vc;
      [m_uiWindow makeKeyAndVisible];

      // Create the AsepriteView inside safe area
      UIEdgeInsets insets = vc.view.safeAreaInsets;
      CGRect safeFrame = CGRectMake(
        insets.left,
        insets.top,
        UIScreen.mainScreen.bounds.size.width - insets.left - insets.right,
        UIScreen.mainScreen.bounds.size.height - insets.top - insets.bottom);
      AsepriteView* aseView = [[AsepriteView alloc] initWithFrame:safeFrame];
      aseView.backgroundColor = UIColor.blackColor;
      aseView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
      [container addSubview:aseView];

      m_scale = (int)UIScreen.mainScreen.scale;
      done = true;
    }
  };

  if ([NSThread isMainThread]) {
    createBlock();
  }
  else {
    dispatch_sync(dispatch_get_main_queue(), createBlock);
  }
}

void WindowIOS::destroyWindow()
{
  if (!m_uiWindow)
    return;

  @autoreleasepool {
    m_uiWindow.hidden = YES;
    m_uiWindow = nil;
  }
}

gfx::Size WindowIOS::clientSize() const
{
  if (m_uiWindow) {
    // Return size in points (like macOS), not pixels
    AsepriteView* view = this->asepriteView();
    if (view) {
      CGRect bounds = view.bounds;
      return gfx::Size(bounds.size.width,
                       bounds.size.height);
    }
    CGRect bounds = m_uiWindow.bounds;
    return gfx::Size(bounds.size.width,
                     bounds.size.height);
  }
  return gfx::Size(0, 0);
}

AsepriteView* WindowIOS::asepriteView() const
{
  if (!m_uiWindow || !m_uiWindow.rootViewController)
    return nil;
  for (UIView* subview in m_uiWindow.rootViewController.view.subviews) {
    if ([subview isKindOfClass:[AsepriteView class]])
      return (AsepriteView*)subview;
  }
  return nil;
}

gfx::Rect WindowIOS::frame() const
{
  if (m_uiWindow) {
    CGRect f = m_uiWindow.frame;
    return gfx::Rect(f.origin.x, f.origin.y,
                     f.size.width, f.size.height);
  }
  return gfx::Rect();
}

void WindowIOS::setFrame(const gfx::Rect& bounds)
{
  // iOS windows are always fullscreen
}

gfx::Rect WindowIOS::contentRect() const
{
  return frame();
}

gfx::Rect WindowIOS::restoredFrame() const
{
  return frame();
}

void WindowIOS::activate()
{
  if (m_uiWindow)
    [m_uiWindow makeKeyAndVisible];
}

std::string WindowIOS::title() const
{
  return "Aseprite";
}

void WindowIOS::setTitle(const std::string& title)
{
  // iOS doesn't have window titles
}

os::ColorSpaceRef WindowIOS::colorSpace() const
{
  return Window::colorSpace();
}

int WindowIOS::scale() const
{
  return m_scale;
}

void WindowIOS::setScale(int scale)
{
  m_scale = scale;
}

bool WindowIOS::isVisible() const
{
  return m_uiWindow && !m_uiWindow.hidden;
}

void WindowIOS::setVisible(bool visible)
{
  if (m_uiWindow) {
    if (visible)
      [m_uiWindow makeKeyAndVisible];
    else
      m_uiWindow.hidden = YES;
  }
}

void* WindowIOS::nativeHandle() const
{
  return (__bridge void*)m_uiWindow;
}

} // namespace os

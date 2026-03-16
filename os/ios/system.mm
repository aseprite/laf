// LAF OS Library
// Copyright (c) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "os/ios/system.h"
#include "os/screen.h"

#import <UIKit/UIKit.h>

namespace os {

class ScreenIOS : public Screen {
public:
  ScreenIOS() {
    @autoreleasepool {
      UIScreen* screen = [UIScreen mainScreen];
      CGRect bounds = screen.bounds;
      // Report bounds in points (like macOS), not pixels
      m_bounds = gfx::Rect(0, 0,
                            (int)(bounds.size.width),
                            (int)(bounds.size.height));
      // Approximate workarea (exclude status bar area)
      m_workarea = m_bounds;
    }
  }

  bool isPrimary() const override { return true; }
  gfx::Rect bounds() const override { return m_bounds; }
  gfx::Rect workarea() const override { return m_workarea; }
  os::ColorSpaceRef colorSpace() const override { return nullptr; }
  void* nativeHandle() const override { return nullptr; }

private:
  gfx::Rect m_bounds;
  gfx::Rect m_workarea;
};

static bool g_keys[kKeyScancodes] = { false };

bool ios_is_key_pressed(KeyScancode scancode)
{
  if (scancode >= 0 && scancode < kKeyScancodes)
    return g_keys[scancode];
  return false;
}

int ios_get_unicode_from_scancode(KeyScancode scancode)
{
  return 0;
}

SystemIOS::~SystemIOS()
{
  destroyInstance();
}

ScreenRef SystemIOS::primaryScreen()
{
  return make_ref<ScreenIOS>();
}

void SystemIOS::listScreens(ScreenList& list)
{
  list.push_back(make_ref<ScreenIOS>());
}

SystemRef System::makeIOS()
{
  return make_ref<SystemIOS>();
}

} // namespace os

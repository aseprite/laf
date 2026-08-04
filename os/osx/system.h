// LAF OS Library
// Copyright (C) 2020-present  Igara Studio S.A.
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_OSX_SYSTEM_H
#define OS_OSX_SYSTEM_H
#pragma once

#include "os/common/system.h"

#include "os/menus.h"
#include "os/osx/app.h"
#include "os/osx/logger.h"
#include "os/osx/menus.h"

namespace os {

bool osx_is_key_pressed(KeyScancode scancode);
int osx_get_unicode_from_scancode(KeyScancode scancode);
void osx_reset_key_pressed();

// By default the NSView will be created with a CALayer for backing
// content with async drawing. This fixes performance issues on Retina
// displays with wide color spaces (like Display P3). But it might
// bring some unknown problems (I think the issues started after macOS
// Catalina was released).
void osx_set_async_view(bool state);

class SystemOSX : public CommonSystem {
public:
  SystemOSX() : m_menus(nullptr) {}
  ~SystemOSX();

  void setAppMode(AppMode appMode) override { AppOSX::instance()->setAppMode(appMode); }

  void markCliFileAsProcessed(const std::string& fn) override
  {
    AppOSX::instance()->markCliFileAsProcessed(fn);
  }

  void finishLaunching() override
  {
    // Start processing NSApplicationDelegate events. (E.g. after
    // calling this we'll receive application:openFiles: and we'll
    // generate DropFiles events.)  events
    AppOSX::instance()->finishLaunching();
  }

  void activateApp() override { AppOSX::instance()->activateApp(); }

  Logger* logger() override { return new LoggerOSX; }

  Menus* menus() override
  {
    if (!m_menus)
      m_menus = make_ref<MenusOSX>();
    return m_menus.get();
  }

  bool isKeyPressed(KeyScancode scancode) override { return osx_is_key_pressed(scancode); }

  void resetKeyPressed() override { osx_reset_key_pressed(); };

  int getUnicodeFromScancode(KeyScancode scancode) override
  {
    return osx_get_unicode_from_scancode(scancode);
  }

  CursorRef makeCursor(const Surface* surface, const gfx::Point& focus, const int scale) override;

  gfx::Point mousePosition() const override;
  void setMousePosition(const gfx::Point& screenPosition) override;
  gfx::Color getColorFromScreen(const gfx::Point& screenPosition) const override;

  ScreenRef primaryScreen() override;
  void listScreens(ScreenList& list) override;

  std::unique_ptr<GpuContext> makePlatformGpuContext() override;

private:
  MenusRef m_menus;
};

} // namespace os

#endif

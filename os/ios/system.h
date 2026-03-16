// LAF OS Library
// Copyright (c) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_IOS_SYSTEM_H
#define OS_IOS_SYSTEM_H
#pragma once

#include "os/common/system.h"

namespace os {

bool ios_is_key_pressed(KeyScancode scancode);
int ios_get_unicode_from_scancode(KeyScancode scancode);

class SystemIOS : public CommonSystem {
public:
  ~SystemIOS();

  void setAppMode(AppMode appMode) override {}
  void markCliFileAsProcessed(const std::string& fn) override {}
  void finishLaunching() override {}
  void activateApp() override {}

  Logger* logger() override { return nullptr; }
  Menus* menus() override { return nullptr; }

  bool isKeyPressed(KeyScancode scancode) override { return ios_is_key_pressed(scancode); }

  int getUnicodeFromScancode(KeyScancode scancode) override
  {
    return ios_get_unicode_from_scancode(scancode);
  }

  CursorRef makeCursor(const Surface* surface, const gfx::Point& focus, const int scale) override
  {
    return nullptr;
  }

  gfx::Point mousePosition() const override { return gfx::Point(0, 0); }
  void setMousePosition(const gfx::Point& screenPosition) override {}
  gfx::Color getColorFromScreen(const gfx::Point& screenPosition) const override
  {
    return gfx::ColorNone;
  }

  ScreenRef primaryScreen() override;
  void listScreens(ScreenList& list) override;
};

} // namespace os

#endif

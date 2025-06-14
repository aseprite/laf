// LAF OS Library
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_COMMON_SYSTEM_H
#define OS_COMMON_SYSTEM_H
#pragma once

#include "os/event_queue.h"
#include "os/menus.h"
#include "os/system.h"

namespace os {

class CommonSystem : public System {
public:
  CommonSystem();
  ~CommonSystem();

  const std::string& appName() const override { return m_appName; }
  void setAppName(const std::string& appName) override { m_appName = appName; }
  void setAppMode(AppMode appMode) override {}

  void markCliFileAsProcessed(const std::string& fn) override {}
  void finishLaunching() override {}
  void activateApp() override {}

  Capabilities capabilities() const override { return (Capabilities)0; }

  // Do nothing options (these functions are for Windows-only at the
  // moment)
  void setTabletOptions(const TabletOptions&) override {}
  TabletOptions tabletOptions() const override { return TabletOptions(); }

  Logger* logger() override { return nullptr; }

  Menus* menus() override { return nullptr; }

  EventQueue* eventQueue() override { return EventQueue::instance(); }

  KeyModifiers keyModifiers() override;
  ScreenRef mainScreen() override { return nullptr; }
  void listScreens(ScreenList& screens) override {}
  Window* defaultWindow() override { return nullptr; }
  Ref<Window> makeWindow(const WindowSpec&) override { return nullptr; }
  Ref<Surface> makeSurface(int, int, const os::ColorSpaceRef&) override { return nullptr; }
#if CLIP_ENABLE_IMAGE
  Ref<Surface> makeSurface(const clip::image& image) override;
#endif
  Ref<Surface> makeRgbaSurface(int, int, const os::ColorSpaceRef&) override { return nullptr; }
  Ref<Surface> loadSurface(const char*) override { return nullptr; }
  Ref<Surface> loadRgbaSurface(const char*) override { return nullptr; }
  Ref<Cursor> makeCursor(const Surface*, const gfx::Point&, int) override { return nullptr; }
  bool isKeyPressed(KeyScancode) override { return false; }
  int getUnicodeFromScancode(KeyScancode) override { return 0; }
  void setTextInput(bool, const gfx::Point&) override {}
  gfx::Point mousePosition() const override { return gfx::Point(0, 0); }
  void setMousePosition(const gfx::Point&) override {}
  gfx::Color getColorFromScreen(const gfx::Point&) const override { return gfx::ColorNone; }
  void listColorSpaces(std::vector<os::ColorSpaceRef>&) override {}
  os::ColorSpaceRef makeColorSpace(const gfx::ColorSpaceRef&) override { return nullptr; }
  Ref<ColorSpaceConversion> convertBetweenColorSpace(const os::ColorSpaceRef&,
                                                     const os::ColorSpaceRef&) override
  {
    return nullptr;
  }
  void setWindowsColorSpace(const os::ColorSpaceRef&) override {}
  os::ColorSpaceRef windowsColorSpace() override { return nullptr; }

protected:
  void destroyInstance();

private:
  std::string m_appName;
};

} // namespace os

#endif

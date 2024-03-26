// LAF OS Library
// Copyright (c) 2018-2021  Igara Studio S.A.
// Copyright (c) 2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/memory.h"
#include "base/string.h"
#include "gfx/size.h"
#include "os/font.h"
#include "os/system.h"
#include "os/window.h"

namespace os {

class NoneSystem : public System {
public:
  void setAppName(const std::string& appName) override { }
  void setAppMode(AppMode appMode) override { }

  void markCliFileAsProcessed(const std::string& fn) override { }
  void finishLaunching() override { }
  void activateApp() override { }

  Capabilities capabilities() const override { return (Capabilities)0; }
  void setTabletAPI(TabletAPI api) override { }
  TabletAPI tabletAPI() const override { return TabletAPI::Default; }
  Logger* logger() override { return nullptr; }
  Menus* menus() override { return nullptr; }
  NativeDialogs* nativeDialogs() override { return nullptr; }
  EventQueue* eventQueue() override { return nullptr; }
  bool gpuAcceleration() const override { return false; }
  void setGpuAcceleration(bool state) override { }
  ScreenRef mainScreen() override { return nullptr; }
  void listScreens(ScreenList& screens) override { }
  Window* defaultWindow() override { return nullptr; }
  Ref<Window> makeWindow(const WindowSpec& spec) override { return nullptr; }
  Ref<Surface> makeSurface(int width, int height,
                           const os::ColorSpaceRef& colorSpace) override { return nullptr; }
  Ref<Surface> makeSurface(int width, int height,
                           const os::SurfaceFormatData& sf,
                           const unsigned char* data = nullptr) override { return nullptr; }
  Ref<Surface> makeRgbaSurface(int width, int height,
                               const os::ColorSpaceRef& colorSpace) override { return nullptr; }
  Ref<Surface> loadSurface(const char* filename) override { return nullptr; }
  Ref<Surface> loadRgbaSurface(const char* filename) override { return nullptr; }
  Ref<Cursor> makeCursor(const Surface* surface,
                         const gfx::Point& focus,
                         const int scale) override { return nullptr; }
  FontManager* fontManager() override { return nullptr; }
  Ref<Font> loadSpriteSheetFont(const char* filename, int scale) override { return nullptr; }
  Ref<Font> loadTrueTypeFont(const char* filename, int height) override { return nullptr; }
  bool isKeyPressed(KeyScancode scancode) override { return false; }
  KeyModifiers keyModifiers() override { return kKeyNoneModifier; }
  int getUnicodeFromScancode(KeyScancode scancode) override { return 0; }
  void setTranslateDeadKeys(bool state) override { }
  gfx::Point mousePosition() const override { return gfx::Point(0, 0); }
  void setMousePosition(const gfx::Point& screenPosition) override { }
  gfx::Color getColorFromScreen(const gfx::Point& screenPosition) const override { return gfx::ColorNone; }
  void listColorSpaces(std::vector<os::ColorSpaceRef>& list) override { }
  os::ColorSpaceRef makeColorSpace(const gfx::ColorSpaceRef& cs) override { return nullptr; }
  Ref<ColorSpaceConversion> convertBetweenColorSpace(
    const os::ColorSpaceRef& src, const os::ColorSpaceRef& dst) override { return nullptr; }
  void setWindowsColorSpace(const os::ColorSpaceRef& cs) override { }
  os::ColorSpaceRef windowsColorSpace() override { return nullptr; }
};

System* make_system_impl() {
  return new NoneSystem;
}

void error_message(const char* msg)
{
  fputs(msg, stderr);
  // TODO
}

} // namespace os

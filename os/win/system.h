// LAF OS Library
// Copyright (C) 2020-2021  Igara Studio S.A.
// Copyright (C) 2012-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_WIN_SYSTEM_H
#define OS_WIN_SYSTEM_H
#pragma once

#include "os/common/system.h"
#include "os/win/winapi.h"
#include "os/win/wintab.h"

namespace os {

class SystemWin : public CommonSystem {
public:
  SystemWin();
  ~SystemWin();

  void setAppMode(AppMode appMode) override;

  WinAPI& winApi() { return m_winApi; }
  WintabAPI& wintabApi() { return m_wintabApi; }

  void setAppName(const std::string& appName) override;
  std::string appName() const { return m_appName; }

  void setTabletAPI(TabletAPI api) override;
  TabletAPI tabletAPI() const override { return m_tabletAPI; }

  bool isKeyPressed(KeyScancode scancode) override;
  int getUnicodeFromScancode(KeyScancode scancode) override;

  CursorRef makeCursor(const Surface* surface,
                       const gfx::Point& focus,
                       const int scale) override;

  gfx::Point mousePosition() const override;
  void setMousePosition(const gfx::Point& screenPosition) override;
  gfx::Color getColorFromScreen(const gfx::Point& screenPosition) const override;

  ScreenRef mainScreen() override;
  void listScreens(ScreenList& list) override;

  void setWintabDelegate(void* delegate) override {
    m_wintabApi.setDelegate((WintabAPI::Delegate*)delegate);
  }

  // Required because GetMousePos() doesn't return the current mouse
  // position when we're using a pen/stylus.
  void _clearInternalMousePosition();
  void _setInternalMousePosition(const gfx::Point& pos);
  void _setInternalMousePosition(const Event& ev);

private:
  std::string m_appName;
  TabletAPI m_tabletAPI = TabletAPI::Default;
  WinAPI m_winApi;
  WintabAPI m_wintabApi;
  gfx::Point m_screenMousePos;
  AppMode m_appMode;
};

} // namespace os

#endif

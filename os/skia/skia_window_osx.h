// LAF OS Library
// Copyright (C) 2018-2022  Igara Studio S.A.
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_SKIA_SKIA_WINDOW_OSX_INCLUDED
#define OS_SKIA_SKIA_WINDOW_OSX_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "os/color_space.h"
#include "os/native_cursor.h"
#include "os/osx/window.h"
#include "os/screen.h"
#include "os/skia/skia_gl.h"
#include "os/skia/skia_window_base.h"

#include <string>

namespace os {

class SkiaWindowOSX : public SkiaWindowBase<WindowOSX> {
public:
  SkiaWindowOSX(const WindowSpec& spec);
  ~SkiaWindowOSX();

  void setFullscreen(bool state) override;

  void invalidateRegion(const gfx::Region& rgn) override;

  std::string getLayout() override { return ""; }
  void setLayout(const std::string& layout) override {}
  void setTextInput(bool state, const gfx::Point& screenCaretPos = {});

  // WindowOSX overrides
  void onClose() override;
  void onResize(const gfx::Size& size) override;
  void onDrawRect(const gfx::Rect& rect) override;
  void onWindowChanged() override;
  void onStartResizing() override;
  void onResizing(gfx::Size& size) override;
  void onEndResizing() override;
  void onChangeBackingProperties() override;

private:
  void paintGC(const gfx::RectF& rect);

  bool m_closing = false;

  // Counter used to match each onStart/EndResizing() call because we
  // can receive multiple calls in case of windowWill/DidEnter/ExitFullScreen
  // and windowWill/DidStart/EndLiveResize notifications.
  int m_resizingCount = 0;

  DISABLE_COPYING(SkiaWindowOSX);
};

} // namespace os

#endif

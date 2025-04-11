// LAF OS Library
// Copyright (C) 2021-2025  Igara Studio S.A.
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_SKIA_SKIA_WINDOW_WIN_INCLUDED
#define OS_SKIA_SKIA_WINDOW_WIN_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "os/skia/skia_window_base.h"
#include "os/win/window.h"

#if SK_SUPPORT_GPU
  #include "include/gpu/ganesh/gl/GrGLInterface.h"
  #include "os/gl/gl_context.h"
#endif

namespace os {

class SkiaWindowWin : public SkiaWindowBase<WindowWin> {
public:
  SkiaWindowWin(const WindowSpec& spec);

  void invalidateRegion(const gfx::Region& rgn) override;

private:
  void onPaint(HDC hdc) override;
  void onStartResizing() override;
  void onEndResizing() override;
  void onChangeColorSpace() override;
  void paintHDC(HDC dc);

  bool m_resizing = false;

  DISABLE_COPYING(SkiaWindowWin);
};

} // namespace os

#endif

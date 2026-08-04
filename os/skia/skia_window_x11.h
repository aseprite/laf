// LAF OS Library
// Copyright (C) 2021-present  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_SKIA_SKIA_WINDOW_X11_INCLUDED
#define OS_SKIA_SKIA_WINDOW_X11_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "gfx/size.h"
#include "os/native_cursor.h"
#include "os/skia/skia_window_base.h"
#include "os/x11/window.h"

#include <string>
#include <vector>

namespace os {

class SkiaWindowX11 : public SkiaWindowBase<WindowX11> {
public:
  SkiaWindowX11(const WindowSpec& spec);

  std::string getLayout() override { return ""; }
  void setLayout(const std::string& layout) override {}

private:
  void onPaint(const gfx::Rect& rc) override;

  std::vector<uint8_t> m_buffer;

  DISABLE_COPYING(SkiaWindowX11);
};

} // namespace os

#endif

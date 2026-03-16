// LAF OS Library
// Copyright (C) 2021  Igara Studio S.A.
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_SKIA_SKIA_WINDOW_INCLUDED
#define OS_SKIA_SKIA_WINDOW_INCLUDED
#pragma once

#if LAF_WINDOWS
  #include "os/skia/skia_window_win.h"
namespace os {
using SkiaWindowPlatform = os::SkiaWindowWin;
}
#elif LAF_IOS
  #include "os/skia/skia_window_ios.h"
namespace os {
using SkiaWindowPlatform = os::SkiaWindowIOS;
}
#elif LAF_MACOS
  #include "os/skia/skia_window_osx.h"
namespace os {
using SkiaWindowPlatform = os::SkiaWindowOSX;
}
#elif LAF_LINUX
  #include "os/skia/skia_window_x11.h"
namespace os {
using SkiaWindowPlatform = os::SkiaWindowX11;
}
#endif

#include "os/native_cursor.h"
#include "os/skia/skia_color_space.h"

namespace os {

class SkiaSurface;
class WindowSpec;

class SkiaWindow : public SkiaWindowPlatform {
public:
  SkiaWindow(const WindowSpec& spec);

  // Returns the real and current window's size (without scale applied).
  int width() const override;
  int height() const override;

  NativeCursor nativeCursor() override;
  bool setCursor(NativeCursor cursor) override;

private:
  NativeCursor m_nativeCursor;
};

} // namespace os

#endif

// LAF OS Library
// Copyright (C) 2020-2022  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_X11_X11_INCLUDED
#define OS_X11_X11_INCLUDED
#pragma once

#include "base/debug.h"
#include "gfx/color_space.h"  // Include here avoid error with None
#include "monitor.h"
#include "os/event_queue.h"

#include <X11/Xlib.h>

#include <memory>

namespace os {

class XInput;

class X11 {
  static X11* m_instance;
public:
  static X11* instance();

  X11();
  ~X11();

  ::Display* display() const { return m_display; }
  ::XIM xim() const { return m_xim; }
  XInput* xinput();
  MonitorsX11* monitors();

private:
  ::Display* m_display;
  ::XIM m_xim;
  std::unique_ptr<XInput> m_xinput;
  std::unique_ptr<MonitorsX11> m_monitors;
};

} // namespace os

#endif

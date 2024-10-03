// LAF OS Library
// Copyright (C) 2021-2022  Igara Studio S.A.
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "os/x11/x11.h"

#include "base/debug.h"
#include "os/x11/event_queue.h"
#include "os/x11/window.h"
#include "os/x11/xinput.h"

namespace os {

X11* X11::m_instance = nullptr;

// static
X11* X11::instance()
{
  ASSERT(m_instance);
  return m_instance;
}

X11::X11()
{
  ASSERT(m_instance == nullptr);
  m_instance = this;

  // TODO We shouldn't need to call this function (because we
  // shouldn't be using the m_display from different threads), but
  // it might be necessary?
  // https://github.com/aseprite/aseprite/issues/1962
  XInitThreads();

  m_display = XOpenDisplay(nullptr);
  m_xim = XOpenIM(m_display, nullptr, nullptr, nullptr);
}

X11::~X11()
{
  ASSERT(m_instance == this);

  // Before closing the X11 display connection, there shouldn't be
  // more windows opened and the event queue (which store
  // Event+WindowRef) should be empty. CommonSystem() dtor clears the
  // queue.
  ASSERT(WindowX11::countActiveWindows() == 0);
  ASSERT(((os::EventQueueX11*)os::EventQueue::instance())->isEmpty());

  if (m_xim) {
    XCloseIM(m_xim);
  }
  if (m_display) {
    if (m_xinput)
      m_xinput->unload(m_display);
    XCloseDisplay(m_display);
  }
  m_instance = nullptr;
}

XInput* X11::xinput()
{
  if (!m_xinput) {
    m_xinput = std::make_unique<XInput>();
    m_xinput->load(m_display);
  }
  return m_xinput.get();
}

MonitorsX11* X11::monitors()
{
  if (!m_monitors) {
    // TODO: Use `XRRScreenChangeNotifyEvent` in the man page of Xrandr(3) to update this variable?
    m_monitors = std::make_unique<MonitorsX11>();
  }

  return m_monitors.get();
}


} // namespace os

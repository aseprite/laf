// LAF OS Library
// Copyright (C) 2021-2024  Igara Studio S.A.
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_X11_SYSTEM_H
#define OS_X11_SYSTEM_H
#pragma once

#include "os/common/system.h"
#include "os/x11/keys.h"
#include "os/x11/screen.h"
#include "os/x11/x11.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>

namespace os {

class SystemX11 : public CommonSystem {
public:
  ~SystemX11();

  void setTabletOptions(const TabletOptions& options) override {
    m_tabletOptions = options;
  }

  TabletOptions tabletOptions() const override {
    return m_tabletOptions;
  }

  bool isKeyPressed(KeyScancode scancode) override {
    return x11_is_key_pressed(scancode);
  }

  int getUnicodeFromScancode(KeyScancode scancode) override {
    return x11_get_unicode_from_scancode(scancode);
  }

  CursorRef getNativeCursor(NativeCursor cursor);

  CursorRef makeCursor(const Surface* surface,
                       const gfx::Point& focus,
                       const int scale) override;

  gfx::Point mousePosition() const override {
    int rootx, rooty, x, y;
    unsigned int mask;
    ::Display* display = X11::instance()->display();
    ::Window root = XDefaultRootWindow(display);
    ::Window child;
    if (!XQueryPointer(display, root, &root, &child, &rootx, &rooty, &x, &y, &mask)) {
      rootx = rooty = 0;
    }
    return gfx::Point(rootx, rooty);
  }

  void setMousePosition(const gfx::Point& screenPosition) override {
    // TODO
  }

  gfx::Color getColorFromScreen(const gfx::Point& screenPosition) const override {
    ::Display* display = X11::instance()->display();
    int screen = XDefaultScreen(display);
    ::Window root = XRootWindow(display, screen);

    XImage* image = XGetImage(display, root,
                              screenPosition.x,
                              screenPosition.y, 1, 1, AllPlanes, ZPixmap);
    if (image) {
      XColor color;
      color.pixel = XGetPixel(image, 0, 0);
      XDestroyImage(image);

      XQueryColor(display, XDefaultColormap(display, screen), &color);

      // Each red/green/blue channel is 16-bit, so we have to convert to 8-bit.
      return gfx::rgba(color.red>>8, color.green>>8, color.blue>>8);
    }
    return gfx::ColorNone;
  }

  ScreenRef mainScreen() override {
    MonitorsX11* monitors = X11::instance()->monitors();
    const int numMonitors = monitors->numMonitors();

    // we have to search for the primary monitor
    for (int monitor=0; monitor<numMonitors; monitor++) {
      if (monitors->monitor(monitor).primary) {
        return make_ref<ScreenX11>(monitor);
      }
    }

    return nullptr;
  }

  void listScreens(ScreenList& list) override {
    const int numMonitors = X11::instance()->monitors()->numMonitors();
    for (int monitor=0; monitor<numMonitors; monitor++)
      list.push_back(make_ref<ScreenX11>(monitor));
  }

private:
  TabletOptions m_tabletOptions;
};

} // namespace os

#endif

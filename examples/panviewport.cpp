// LAF Library
// Copyright (C) 2020-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "gfx/hsv.h"
#include "gfx/rgb.h"
#include "os/os.h"
#include "text/text.h"

#include "examples/toggle_gpu.h"

#include <algorithm>
#include <cstdarg>
#include <cstdlib>
#include <string>
#include <vector>

using namespace os;
using namespace text;

class PanWindow {
public:
  PanWindow(os::System* system)
    : m_window(system->makeWindow(800, 600))
    , m_font(FontMgr::Make()->defaultFont(12))
    , m_scroll(0.0, 0.0)
    , m_zoom(1.0)
    , m_hasCapture(false)
  {
    m_window->setCursor(os::NativeCursor::Arrow);
    m_window->setTitle("Pan Viewport");
    repaint();
    m_window->setVisible(true);
  }

  bool processEvent(const os::Event& ev)
  {
    if (handle_toggle_gpu_key(ev))
      return true;

    switch (ev.type()) {
      case os::Event::CloseWindow:  return false;

      case os::Event::ResizeWindow: repaint(); break;

      case os::Event::MouseEnter:   break;
      case os::Event::MouseLeave:   break;
      case os::Event::MouseMove:
        if (m_hasCapture) {
          m_scroll = m_captureScroll + gfx::PointF(ev.position() - m_capturePos);
          repaint();
        }
        break;

      case os::Event::MouseDown:
        if (!m_hasCapture) {
          m_window->setCursor(os::NativeCursor::Move);
          m_window->captureMouse();
          m_hasCapture = true;
          m_capturePos = ev.position();
          m_captureScroll = m_scroll;
        }
        break;

      case os::Event::MouseUp:
        if (m_hasCapture) {
          m_window->setCursor(os::NativeCursor::Arrow);
          m_window->releaseMouse();
          m_hasCapture = false;
        }
        break;

      case os::Event::MouseDoubleClick:
        // Reset
        m_scroll = gfx::PointF(0.0, 0.0);
        m_zoom = 1.0;
        repaint();
        break;

      case os::Event::MouseWheel:
        if (ev.modifiers() & os::kKeyCtrlModifier) {
          int z = (ev.wheelDelta().x + ev.wheelDelta().y);
          setZoom(gfx::PointF(ev.position()), m_zoom - z / 10.0);
        }
        else if (ev.preciseWheel()) {
          // TODO we have plans to change the sign of wheelDelta() when preciseWheel() is true
          m_scroll += gfx::PointF(-ev.wheelDelta());
        }
        else {
          m_scroll += gfx::PointF(-ev.wheelDelta().x * m_window->width() / 32,
                                  -ev.wheelDelta().y * m_window->height() / 32);
        }
        repaint();
        break;

      case os::Event::TouchMagnify:
        setZoom(gfx::PointF(ev.position()), m_zoom + m_zoom * ev.magnification());
        break;

      case os::Event::KeyDown:
        switch (ev.scancode()) {
          case os::kKeyEsc:
            return false;

            // Toggle full-screen
          case os::kKeyF:
          case os::kKeyF11: m_window->setFullscreen(!m_window->isFullscreen()); break;
        }
        break;

      default:
        // Do nothing
        break;
    }
    return true;
  }

  void repaint()
  {
    os::Surface* surface = m_window->surface();
    const gfx::Rect rc(surface->bounds());

    os::Paint p;
    p.style(os::Paint::Fill);
    p.color(gfx::rgba(32, 32, 32, 255));
    surface->drawRect(rc, p);

    p.style(os::Paint::Stroke);
    p.color(gfx::rgba(255, 255, 200, 255));
    {
      gfx::RectF rc2(rc);
      rc2.shrink(24);
      rc2.offset(-center());
      rc2 *= m_zoom;
      rc2.offset(center());
      rc2.offset(m_scroll);
      surface->drawRect(rc2, p);

      for (int i = 1; i < 8; ++i) {
        int v = i * rc2.w / 8;
        surface->drawLine(int(rc2.x + v), int(rc2.y), int(rc2.x + v), int(rc2.y + rc2.h), p);
        v = i * rc2.h / 8;
        surface->drawLine(int(rc2.x), int(rc2.y + v), int(rc2.x + rc2.w), int(rc2.y + v), p);
      }
    }

    {
      std::vector<char> buf(256);
      std::snprintf(buf.data(),
                    buf.size(),
                    "Scroll=%.2f %.2f  Zoom=%.2f",
                    m_scroll.x,
                    m_scroll.y,
                    m_zoom);
      p.style(os::Paint::Fill);
      draw_text(surface, m_font, &buf[0], gfx::Point(12, 12), &p);
    }

    m_window->invalidate();
    m_window->swapBuffers();
  }

private:
  void setZoom(const gfx::PointF& mousePos, double newZoom)
  {
    double oldZoom = m_zoom;
    m_zoom = std::clamp(newZoom, 0.01, 10.0);

    // To calculate the new scroll value (m_scroll), we know that the
    // mouse position (mousePos) will be the same with the old and the
    // new scroll/zoom values.
    //
    // So:
    //   pivot    = ((mousePos - m_scroll) - center()) / oldZoom
    //   mousePos = (pivot*m_zoom + center() + m_scroll)
    //
    // Then:
    //   mousePos = (mousePos - m_scroll - center()) * m_zoom / oldZoom + center() + m_scroll
    //   m_scroll = mousePos - (mousePos - m_scroll - center()) * m_zoom / oldZoom - center()
    m_scroll = mousePos - (mousePos - m_scroll - center()) * m_zoom / oldZoom - center();

    repaint();
  }

  gfx::Point center() const { return gfx::Point(m_window->width() / 2, m_window->height() / 2); }

  os::WindowRef m_window;
  FontRef m_font;
  gfx::PointF m_scroll;
  double m_zoom;

  // To pan the viewport with drag & drop
  bool m_hasCapture;
  gfx::Point m_capturePos;
  gfx::PointF m_captureScroll;
};

int app_main(int argc, char* argv[])
{
  SystemRef system = System::make();
  system->setAppMode(AppMode::GUI);

  PanWindow window(system.get());

  system->handleWindowResize = [&window](os::Window* win) { window.repaint(); };

  system->finishLaunching();
  system->activateApp();

  os::EventQueue* queue = system->eventQueue();
  while (true) {
    os::Event ev;
    queue->getEvent(ev);
    if (!window.processEvent(ev))
      break;
  }

  return 0;
}

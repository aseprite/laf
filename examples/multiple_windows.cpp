// LAF Library
// Copyright (c) 2019-present  Igara Studio S.A.
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
#include <cstdio>
#include <vector>

using namespace os;
using namespace text;

static std::vector<WindowRef> windows;

const char* lines[] = { "A: Switch mouse cursor to Arrow <-> Move",
                        "H: Hide window (or show all windows again)",
                        "",
                        "C: Change window frame to content",
                        "F: Change window content to frame",
                        "W: Change window content to workarea",
                        "",
                        "D: Duplicate window",
                        "",
                        "Arrows: Move window",
                        "Shift+Arrows: Resize window",
                        "",
                        "Q: Close all windows",
                        "ESC: Close this window" };

static void redraw_window(Window* window, const FontRef& font)
{
  window->makeCurrent();

  Surface* s = window->surface();
  Paint paint;
  paint.color(gfx::rgba(0, 0, 0));
  s->drawRect(window->bounds(), paint);

  paint.color(gfx::rgba(255, 255, 255));

  char buf[256];
  int y = 12;

  gfx::Rect rc = window->frame();
  std::snprintf(buf, sizeof(buf), "Frame = (%d %d %d %d)", rc.x, rc.y, rc.w, rc.h);
  draw_text(s, font, buf, gfx::Point(0, y), &paint);
  y += 12;

  rc = window->contentRect();
  std::snprintf(buf, sizeof(buf), "Content Rect = (%d %d %d %d)", rc.x, rc.y, rc.w, rc.h);
  draw_text(s, font, buf, gfx::Point(0, y), &paint);
  y += 12;

  for (auto line : lines) {
    y += 12;
    draw_text(s, font, line, gfx::Point(0, y), &paint);
  }

  paint.style(Paint::Style::Stroke);
  s->drawRect(window->bounds(), paint);
}

static WindowRef add_window(const std::string& title, const WindowSpec& spec, const FontRef& font)
{
  WindowRef newWindow = System::instance()->makeWindow(spec);
  newWindow->setCursor(NativeCursor::Arrow);
  newWindow->setTitle(title);
  windows.emplace_back(newWindow);

  redraw_window(newWindow.get(), font);
  newWindow->setVisible(true);
  return newWindow;
}

static void check_show_all_windows()
{
  // If all windows are hidden, show then again
  auto hidden = std::count_if(windows.begin(), windows.end(), [](WindowRef window) {
    return !window->isVisible();
  });
  if (hidden == windows.size()) {
    std::for_each(windows.begin(), windows.end(), [](WindowRef window) {
      window->setVisible(true);
    });
  }
}

static void destroy_window(const WindowRef& window)
{
  auto it = std::find(windows.begin(), windows.end(), window);
  if (it != windows.end())
    windows.erase(it);

  check_show_all_windows();
}

int app_main(int argc, char* argv[])
{
  SystemRef system = System::make();
  FontRef font = FontMgr::Make()->defaultFont(12);

  system->setAppMode(AppMode::GUI);
  system->handleWindowResize = [&font](Window* w) { redraw_window(w, font); };
  system->handleWindowMoving = [&font](Window* w) {
    redraw_window(w, font);
    w->invalidate();
    w->swapBuffers();
  };

  // Create four windows for each screen with the bounds of the
  // workarea.
  ScreenList screens;
  system->listScreens(screens);
  char chr = 'A';
  for (ScreenRef& screen : screens) {
    const std::string primary = (screen->isPrimary() ? std::string(" (primary screen)") :
                                                       std::string());

    WindowSpec spec;
    spec.titled(true);
    spec.position(WindowSpec::Position::Frame);
    spec.frame(screen->workarea());
    spec.screen(screen);

    gfx::Point pos[4] = {
      { 0, 0 },
      { 1, 0 },
      { 0, 1 },
      { 1, 1 }
    };
    for (auto& p : pos) {
      WindowSpec s = spec;
      gfx::Rect frame = s.frame();
      frame.w /= 2;
      frame.h /= 2;
      frame.x = frame.x + frame.w * p.x;
      frame.y = frame.y + frame.h * p.y;
      s.frame(frame);
      add_window(std::string(1, chr++) + primary, s, font);
    }
  }

  system->finishLaunching();
  system->activateApp();

  EventQueue* queue = system->eventQueue();
  Event ev;
  while (!windows.empty()) {
    queue->getEvent(ev);

    if (handle_toggle_gpu_key(ev))
      continue;

    switch (ev.type()) {
      case Event::CloseApp:
        windows.clear(); // Close all windows
        break;

      case Event::CloseWindow: destroy_window(ev.window()); break;

      case Event::ResizeWindow:
        redraw_window(ev.window().get(), font);
        ev.window()->invalidate();
        break;

      case Event::KeyDown:
        switch (ev.scancode()) {
          case kKeyQ:   windows.clear(); break;

          case kKeyEsc: destroy_window(ev.window()); break;

          // Switch between Arrow/Move cursor in this specific window
          case kKeyA:
            ev.window()->setCursor(ev.window()->nativeCursor() == NativeCursor::Arrow ?
                                     NativeCursor::Move :
                                     NativeCursor::Arrow);
            break;

          case kKeyH:
            ev.window()->setVisible(!ev.window()->isVisible());
            check_show_all_windows();
            break;

          // Duplicate window
          case kKeyD: {
            std::string title = ev.window()->title();
            WindowSpec spec;
            spec.position(WindowSpec::Position::Frame);
            spec.frame(ev.window()->frame());
            add_window(title, spec, font);
            break;
          }

          case kKeyF:
          case kKeyC:
          case kKeyW: {
            std::string title = ev.window()->title();
            WindowSpec spec;
            if (ev.scancode() == kKeyF) {
              spec.position(WindowSpec::Position::ContentRect);
              spec.contentRect(ev.window()->frame());
            }
            else if (ev.scancode() == kKeyC) {
              spec.position(WindowSpec::Position::Frame);
              spec.frame(ev.window()->contentRect());
            }
            else if (ev.scancode() == kKeyW) {
              spec.position(WindowSpec::Position::Frame);
              spec.frame(ev.window()->screen()->workarea());
            }

            // TODO add a new Window::setSpec() method instead of re-creating window
            destroy_window(ev.window());
            add_window(title, spec, font);
            break;
          }

          // With arrow keys we can thest the Window::setFrame() function
          case kKeyLeft:
          case kKeyUp:
          case kKeyRight:
          case kKeyDown:  {
            gfx::Rect rc = ev.window()->frame();
            if (ev.modifiers() & kKeyShiftModifier) {
              switch (ev.scancode()) {
                case kKeyLeft:  rc.w /= 2; break;
                case kKeyUp:    rc.h /= 2; break;
                case kKeyRight: rc.w *= 2; break;
                case kKeyDown:  rc.h *= 2; break;
                default:        break;
              }
            }
            else {
              switch (ev.scancode()) {
                case kKeyLeft:  rc.x -= rc.w; break;
                case kKeyUp:    rc.y -= rc.h; break;
                case kKeyRight: rc.x += rc.w; break;
                case kKeyDown:  rc.y += rc.h; break;
                default:        break;
              }
            }
            ev.window()->setFrame(rc);

            // Redraw window because so we can show the new position
            // on it
            redraw_window(ev.window().get(), font);
            ev.window()->invalidate();
            break;
          }

          default: break;
        }
        break;

      default:
        // Do nothing
        break;
    }
  }

  return 0;
}

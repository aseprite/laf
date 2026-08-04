// LAF Library
// Copyright (c) 2019-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "os/os.h"

#include "examples/toggle_gpu.h"

using namespace os;

void draw_window(Window* window)
{
  Surface* surface = window->surface();
  const gfx::Rect rc = surface->bounds();

  Paint p;
  p.color(gfx::rgba(0, 0, 0));
  p.style(Paint::Fill);
  surface->drawRect(rc, p);

  p.color(gfx::rgba(255, 0, 0));
  surface->drawLine(0, 0, rc.w, rc.h, p);
  p.color(gfx::rgba(0, 128, 0));
  surface->drawLine(rc.w / 2, 0, rc.w / 2, rc.h, p);
  p.color(gfx::rgba(0, 0, 255));
  surface->drawLine(rc.w, 0, 0, rc.h, p);

  window->invalidateRegion(gfx::Region(rc));
  window->swapBuffers();
}

int app_main(int argc, char* argv[])
{
  SystemRef system = System::make();
  system->setAppMode(AppMode::GUI);

  WindowRef window = system->makeWindow(400, 300);

  // Set the title bar caption of the native window.
  window->setTitle("Hello World");

  // We can change the cursor to use when the mouse is above this
  // window, this line is not required because by default the native
  // cursor to be shown in a window is the arrow.
  window->setCursor(NativeCursor::Arrow);

  system->handleWindowResize = draw_window;

  // On macOS: With finishLaunching() we start processing
  // NSApplicationDelegate events. After calling this we'll start
  // receiving Event::DropFiles events. It's a way to say "ok
  // we're ready to process messages"
  system->finishLaunching();

  // On macOS, when we compile the program outside an app bundle, we
  // must active the app explicitly if we want to put the app on the
  // front. Remove this if you're planning to distribute your app on a
  // bundle or enclose it in something like #ifdef _DEBUG/#endif
  system->activateApp();

  // Wait until a key is pressed or the window is closed
  EventQueue* queue = system->eventQueue();
  bool running = true;
  bool redraw = true;
  while (running) {
    if (redraw) {
      const bool isVisible = window->isVisible();

      redraw = false;
      draw_window(window.get());

      if (!isVisible)
        window->setVisible(true);
    }
    // Wait for an event in the queue, the "true" parameter indicates
    // that we'll wait for a new event, and the next line will not be
    // processed until we receive a new event. If we use "false" and
    // there is no events in the queue, we receive an "ev.type() == Event::None
    Event ev;
    queue->getEvent(ev);

    if (handle_toggle_gpu_key(ev)) {
      redraw = true;
      continue;
    }

    switch (ev.type()) {
      case Event::CloseApp:
      case Event::CloseWindow: running = false; break;

      case Event::KeyDown:
        switch (ev.scancode()) {
          case kKeyEsc: running = false; break;

          case kKey1:
          case kKey2:
          case kKey3:
          case kKey4:
          case kKey5:
          case kKey6:
          case kKey7:
          case kKey8:
          case kKey9:
            // Set scale
            window->setScale(1 + (int)(ev.scancode() - kKey1));
            redraw = true;
            break;

          case kKeyF:
          case kKeyF11: window->setFullscreen(!window->isFullscreen()); break;

          default:
            // Do nothing
            break;
        }
        break;

      case Event::ResizeWindow: redraw = true; break;

      default:
        // Do nothing
        break;
    }
  }

  return 0;
}

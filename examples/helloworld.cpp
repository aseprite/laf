// LAF Library
// Copyright (c) 2019-2022  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "os/os.h"

void draw_window(os::Window* window)
{
  os::Surface* surface = window->surface();
  os::SurfaceLock lock(surface);
  const gfx::Rect rc = surface->bounds();

  os::Paint p;
  p.color(gfx::rgba(0, 0, 0));
  p.style(os::Paint::Fill);
  surface->drawRect(rc, p);

  p.color(gfx::rgba(255, 0, 0)); surface->drawLine(0     , 0,   rc.w, rc.h, p);
  p.color(gfx::rgba(0, 128, 0)); surface->drawLine(rc.w/2, 0, rc.w/2, rc.h, p);
  p.color(gfx::rgba(0, 0, 255)); surface->drawLine(rc.w  , 0,      0, rc.h, p);
  p.color(gfx::rgba(255, 255, 255));
  os::draw_text(surface, nullptr, "Hello World", rc.center(),
                &p, os::TextAlign::Center);

  if (window->isGpuAccelerated())
    os::draw_text(surface, nullptr, "(GPU)", rc.center()+gfx::Point(0, 24),
                  &p, os::TextAlign::Center);

  // Invalidates the whole window to show it on the screen.
  if (window->isVisible())
    window->invalidateRegion(gfx::Region(rc));
  else
    window->setVisible(true);

  window->swapBuffers();
}

int app_main(int argc, char* argv[])
{
  int* ptr = nullptr;
  *ptr = 5;

  os::SystemRef system = os::make_system();
  system->setAppMode(os::AppMode::GUI);

  os::WindowRef window = system->makeWindow(400, 300);

  // Set the title bar caption of the native window.
  window->setTitle("Hello World");

  // We can change the cursor to use when the mouse is above this
  // window, this line is not required because by default the native
  // cursor to be shown in a window is the arrow.
  window->setCursor(os::NativeCursor::Arrow);

  system->handleWindowResize = draw_window;

  // On macOS: With finishLaunching() we start processing
  // NSApplicationDelegate events. After calling this we'll start
  // receiving os::Event::DropFiles events. It's a way to say "ok
  // we're ready to process messages"
  system->finishLaunching();

  // On macOS, when we compile the program outside an app bundle, we
  // must active the app explicitly if we want to put the app on the
  // front. Remove this if you're planning to distribute your app on a
  // bundle or enclose it in something like #ifdef _DEBUG/#endif
  system->activateApp();

  // Wait until a key is pressed or the window is closed
  os::EventQueue* queue = system->eventQueue();
  bool running = true;
  bool redraw = true;
  while (running) {
    if (redraw) {
      redraw = false;
      draw_window(window.get());
    }
    // Wait for an event in the queue, the "true" parameter indicates
    // that we'll wait for a new event, and the next line will not be
    // processed until we receive a new event. If we use "false" and
    // there is no events in the queue, we receive an "ev.type() == Event::None
    os::Event ev;
    queue->getEvent(ev);

    switch (ev.type()) {

      case os::Event::CloseApp:
      case os::Event::CloseWindow:
        running = false;
        break;

      case os::Event::KeyDown:
        switch (ev.scancode()) {
          case os::kKeyEsc:
            running = false;
            break;

          case os::kKeyG:
            system->setGpuAcceleration(!system->gpuAcceleration());
            // TODO change window backend immediately
            redraw = true;
            break;

          case os::kKey1:
          case os::kKey2:
          case os::kKey3:
          case os::kKey4:
          case os::kKey5:
          case os::kKey6:
          case os::kKey7:
          case os::kKey8:
          case os::kKey9:
            // Set scale
            window->setScale(1 + (int)(ev.scancode() - os::kKey1));
            redraw = true;
            break;

          case os::kKeyF:
          case os::kKeyF11:
            window->setFullscreen(!window->isFullscreen());
            break;

          default:
            // Do nothing
            break;
        }
        break;

      case os::Event::ResizeWindow:
        redraw = true;
        break;

      default:
        // Do nothing
        break;
    }
  }

  return 0;
}

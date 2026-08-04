// LAF Library
// Copyright (c) 2024-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "base/paths.h"
#include "os/os.h"
#include "text/text.h"

#include "examples/toggle_gpu.h"

#include <cstdio>
#include <string>

struct WindowData {
  bool dragEnter;
  bool dragLeave;
  int drag;
  base::paths paths;
  os::SurfaceRef image;
  std::string url;
  gfx::Point dragPosition;
  gfx::Rect dropZone;
};

static text::FontRef font;
static WindowData windowData;

static void redraw_window(os::Window* window);

class DragTarget : public os::DragTarget {
public:
  void dragEnter(os::DragEvent& ev) override
  {
    windowData.dragEnter = true;
    windowData.dragLeave = false;
    windowData.drag = 0;
    windowData.dragPosition = ev.position();
    redraw_window(ev.target());
  }

  void dragLeave(os::DragEvent& ev) override
  {
    windowData.dragEnter = false;
    windowData.dragLeave = true;
    windowData.dragPosition = ev.position();
    redraw_window(ev.target());
  }

  void drag(os::DragEvent& ev) override
  {
    if (!windowData.dropZone.contains(ev.position()) ||
        !ev.sourceSupports(os::DropOperation::Copy)) {
      ev.dropResult(os::DropOperation::None);
    }
    else if (ev.sourceSupports(os::DropOperation::Copy)) {
      ev.dropResult(os::DropOperation::Copy);
    }

    ++windowData.drag;
    windowData.dragPosition = ev.position();
    redraw_window(ev.target());
  }

  void drop(os::DragEvent& ev) override
  {
    windowData.dragEnter = false;
    windowData.dragLeave = false;
    windowData.dragPosition = { 0, 0 };
    ev.acceptDrop(windowData.dropZone.contains(ev.position()));

    if (ev.acceptDrop()) {
      if (ev.dataProvider()->contains(os::DragDataItemType::Paths))
        windowData.paths = ev.dataProvider()->getPaths();
      else
        windowData.paths.clear();

#if CLIP_ENABLE_IMAGE
      if (ev.dataProvider()->contains(os::DragDataItemType::Image))
        windowData.image = ev.dataProvider()->getImage();
      else
        windowData.image.reset();
#endif

      if (ev.dataProvider()->contains(os::DragDataItemType::Url))
        windowData.url = ev.dataProvider()->getUrl();
      else
        windowData.url.clear();
    }

    redraw_window(ev.target());
  }
};

static void redraw_window(os::Window* window)
{
  os::Surface* s = window->surface();
  const gfx::Rect rc = s->bounds();

  os::Paint paint;
  paint.color(gfx::rgba(32, 32, 32, 255));
  s->drawRect(rc, paint);

  paint.color(gfx::rgba(255, 255, 200, 255));

  char buf[2049];
  int y = 12;
  std::snprintf(buf,
                sizeof(buf),
                "Drag Position = [%d, %d]",
                windowData.dragPosition.x,
                windowData.dragPosition.y);
  text::draw_text(s, font, buf, gfx::Point(0, y), &paint);
  y += 12;
  std::snprintf(buf, sizeof(buf), "Drag Enter = %s", windowData.dragEnter ? "true" : "false");
  text::draw_text(s, font, buf, gfx::Point(0, y), &paint);
  y += 12;
  std::snprintf(buf, sizeof(buf), "Drag = %d", windowData.drag);
  text::draw_text(s, font, buf, gfx::Point(0, y), &paint);
  y += 12;
  std::snprintf(buf, sizeof(buf), "Drag Leave = %s", windowData.dragLeave ? "true" : "false");
  text::draw_text(s, font, buf, gfx::Point(0, y), &paint);

  if (!windowData.paths.empty()) {
    y += 12;
    std::snprintf(buf, sizeof(buf), "Paths = %lu", windowData.paths.size());
    text::draw_text(s, font, buf, gfx::Point(0, y), &paint);
    for (const auto& path : windowData.paths) {
      y += 12;
      std::snprintf(buf, sizeof(buf), "%s", path.c_str());
      text::draw_text(s, font, buf, gfx::Point(12, y), &paint);
    }
  }

  if (!windowData.url.empty()) {
    y += 12;
    std::snprintf(buf, sizeof(buf), "URL:");
    text::draw_text(s, font, buf, gfx::Point(0, y), &paint);
    y += 12;
    std::snprintf(buf, sizeof(buf), "%s", windowData.url.c_str());
    text::draw_text(s, font, buf, gfx::Point(12, y), &paint);
  }

  if (windowData.image) {
    y += 12;
    s->drawRgbaSurface(windowData.image.get(), 0, y);
  }

  paint.style(os::Paint::Style::Stroke);
  s->drawRect(rc, paint);

  auto zoneColor = paint.color();
  auto textColor = zoneColor;

  windowData.dropZone.x = rc.w - windowData.dropZone.w - 12;
  if (windowData.dropZone.contains(windowData.dragPosition)) {
    paint.style(os::Paint::Style::Fill);
    paint.color(zoneColor);
    s->drawRect(windowData.dropZone, paint);
    textColor = gfx::rgba(0, 0, 0);
  }

  paint.color(zoneColor);
  paint.style(os::Paint::Style::Stroke);
  s->drawRect(windowData.dropZone, paint);

  paint.color(textColor);
  paint.style(os::Paint::Style::Fill);
  text::draw_text(s,
                  font,
                  "Drop here!",
                  windowData.dropZone.center(),
                  &paint,
                  text::TextAlign::Center);

  if (window->isVisible()) {
    window->invalidateRegion(gfx::Region(rc));
    window->swapBuffers();
  }
  else
    window->setVisible(true);
}

static os::WindowRef create_window(os::DragTarget& dragTarget)
{
  auto system = os::System::instance();
  auto screen = system->primaryScreen();
  os::WindowSpec spec;
  spec.titled(true);
  spec.position(os::WindowSpec::Position::Frame);
  spec.frame(screen->workarea() / 2);
  spec.screen(screen);
  spec.scale(2);

  os::WindowRef newWindow = system->makeWindow(spec);
  newWindow->setCursor(os::NativeCursor::Arrow);
  newWindow->setTitle("Drag & Drop");
  newWindow->setDragTarget(&dragTarget);

  windowData.dropZone = gfx::Rect(spec.frame().w - 64 - 12, 12, 64, 64);
  redraw_window(newWindow.get());
  return newWindow;
}

int app_main(int argc, char* argv[])
{
  auto system = os::System::make();

  system->setAppMode(os::AppMode::GUI);
  system->handleWindowResize = redraw_window;

  font = text::FontMgr::Make()->defaultFont();

  DragTarget dragTarget;
  os::WindowRef window = create_window(dragTarget);

  bool running = true;

  system->finishLaunching();
  system->activateApp();

  os::EventQueue* queue = system->eventQueue();
  os::Event ev;
  while (running) {
    queue->getEvent(ev);

    if (handle_toggle_gpu_key(ev)) {
      redraw_window(ev.window().get());
      continue;
    }

    switch (ev.type()) {
      case os::Event::KeyDown:
        switch (ev.scancode()) {
          case os::kKeyEsc: running = false; break;
        }
        break;

      case os::Event::CloseApp:
      case os::Event::CloseWindow:  running = false; break;

      case os::Event::ResizeWindow: redraw_window(ev.window().get()); break;

      default:
        // Do nothing
        break;
    }
  }

  return 0;
}

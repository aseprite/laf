// LAF Library
// Copyright (c) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.


#include "base/paths.h"
#include "gfx/hsv.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/rgb.h"
#include "os/dnd.h"
#include "os/draw_text.h"
#include "os/os.h"
#include "os/paint.h"
#include "os/surface.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <map>
#include <memory>
#include <vector>

using Boxes = std::vector<gfx::Rect>;

struct WindowData {
  bool dragEnter;
  bool dragLeave;
  int drag;
  base::paths paths;
  os::SurfaceRef image;
  gfx::Point dragPosition;
  gfx::Rect dropZone;
};

static WindowData windowData;

static void redraw_window(os::Window* window);

class DragTarget : public os::DragTarget {
public:
    void dragEnter(os::DragEvent& ev) override {
      if (!windowData.dropZone.contains(ev.position()) || !ev.sourceSupports(os::DropOperation::Copy))
        ev.dropResult(os::DropOperation::None);
      else if (ev.sourceSupports(os::DropOperation::Copy))
        ev.dropResult(os::DropOperation::Copy);

      windowData.dragEnter = true;
      windowData.dragLeave = false;
      windowData.drag = 0;
      windowData.dragPosition = ev.position();
      redraw_window(ev.target());
      ev.target()->invalidate();
    }
    void dragLeave(os::DragEvent& ev) override {
      windowData.dragEnter = false;
      windowData.dragLeave = true;
      windowData.dragPosition = ev.position();
      redraw_window(ev.target());
      ev.target()->invalidate();
    }
    void drag(os::DragEvent& ev) override {
      ++windowData.drag;
      windowData.dragPosition = ev.position();
      redraw_window(ev.target());
      ev.target()->invalidate();
    }
    void drop(os::DragEvent& ev) override {
      windowData.dragEnter = false;
      windowData.dragLeave = false;
      windowData.dragPosition = {0, 0};
      ev.acceptDrop(windowData.dropZone.contains(ev.position()));

      if (ev.acceptDrop()) {
        if (ev.dataProvider()->contains(os::DragDataItemType::Paths))
          windowData.paths = ev.dataProvider()->getPaths();
        if (ev.dataProvider()->contains(os::DragDataItemType::Image))
          windowData.image = ev.dataProvider()->getImage();
      }

      redraw_window(ev.target());
      ev.target()->invalidate();
    }
};

static void redraw_window(os::Window* window)
{
  os::Surface* s = window->surface();
  os::Paint paint;
  paint.color(gfx::rgba(0, 0, 0));
  s->drawRect(window->bounds(), paint);

  paint.color(gfx::rgba(255, 255, 255));

  char buf[256];
  int y = 12;
  std::snprintf(buf, sizeof(buf), "Drag Position = [%d, %d]", windowData.dragPosition.x, windowData.dragPosition.y);
  os::draw_text(s, nullptr, buf, gfx::Point(0, y), &paint);
  y += 12;
  std::snprintf(buf, sizeof(buf), "Drag Enter = %s", windowData.dragEnter ? "true" : "false");
  os::draw_text(s, nullptr, buf, gfx::Point(0, y), &paint);
  y += 12;
  std::snprintf(buf, sizeof(buf), "Drag = %d", windowData.drag);
  os::draw_text(s, nullptr, buf, gfx::Point(0, y), &paint);
  y += 12;
  std::snprintf(buf, sizeof(buf), "Drag Leave = %s", windowData.dragLeave ? "true" : "false");
  os::draw_text(s, nullptr, buf, gfx::Point(0, y), &paint);

  if (!windowData.paths.empty()) {
    y += 12;
    std::snprintf(buf, sizeof(buf), "Paths = %lu", windowData.paths.size());
    os::draw_text(s, nullptr, buf, gfx::Point(0, y), &paint);
    for (const auto& path : windowData.paths) {
      y += 12;
      std::snprintf(buf, sizeof(buf), "%s", path.c_str());
      os::draw_text(s, nullptr, buf, gfx::Point(12, y), &paint);
    }
  }

  if (windowData.image) {
    y += 12;
    s->drawSurface(windowData.image.get(), 0, y);
  }

  paint.style(os::Paint::Style::Stroke);
  s->drawRect(window->bounds(), paint);


  auto zoneColor = gfx::rgba(100, 255, 100);
  auto textColor = zoneColor;
  if (windowData.dropZone.contains(windowData.dragPosition)){
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
  os::draw_text(s, nullptr, "Drop here!", windowData.dropZone.center(), &paint, os::TextAlign::Center);
}

static os::WindowRef create_window(const std::string& title,
                                  const os::WindowSpec& spec,
                                  os::DragTarget& dragTarget)
{
  os::WindowRef newWindow = os::instance()->makeWindow(spec);
  newWindow->setCursor(os::NativeCursor::Arrow);
  newWindow->setTitle(title);
  newWindow->setVisible(true);
  newWindow->setDragTarget(&dragTarget);
  windowData.dropZone = {32, spec.frame().h - 64 - 40, 64, 64};
  redraw_window(newWindow.get());
  return newWindow;
}

int app_main(int argc, char* argv[])
{
  auto system = os::make_system();
  system->setAppMode(os::AppMode::GUI);
  system->handleWindowResize = redraw_window;

  auto screen = system->mainScreen();
  os::WindowSpec spec;
  DragTarget dragTarget;
  spec.titled(true);
  spec.position(os::WindowSpec::Position::Frame);
  auto frame = screen->workarea()/2;
  spec.frame(frame);
  spec.screen(screen);
  os::WindowRef window = create_window("Drag & Drop example", spec, dragTarget);

  bool running = true;

  system->finishLaunching();
  system->activateApp();

  os::EventQueue* queue = system->eventQueue();
  os::Event ev;
  while (running) {
    queue->getEvent(ev);

    switch (ev.type()) {

      case os::Event::CloseApp:
      case os::Event::CloseWindow:
        running = false;
        break;

      case os::Event::ResizeWindow:
        redraw_window(ev.window().get());
        ev.window()->invalidate();
        break;

      default:
        // Do nothing
        break;
    }
  }

  return 0;
}

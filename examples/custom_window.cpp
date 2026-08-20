// LAF Library
// Copyright (c) 2021-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "os/os.h"
#include "text/text.h"

#include "examples/toggle_gpu.h"

using namespace os;
using namespace text;

const int kTitleBarSize = 32;
const int kButtonSize = 32;
const int kResizeBorder = 8;

const gfx::Color kTitleBarBase = gfx::rgba(100, 100, 200);
const gfx::Color kTitleBarHigh = gfx::rgba(200, 200, 220);
const gfx::Color kTitleBarText = gfx::rgba(25, 25, 50);
const gfx::Color kContentBase = gfx::rgba(40, 40, 35);
const gfx::Color kContentHigh = gfx::rgba(50, 55, 45);
const gfx::Color kContentText = gfx::rgba(105, 115, 85);
const gfx::Color kContentEdge = gfx::rgba(200, 200, 100);

Hit hit_test(Window* window, const gfx::Point& pos)
{
  // For a full screen window, we are always in the content area
  if (window->isFullscreen())
    return Hit::Content;

  gfx::Rect rc = window->bounds() / window->scale();
  gfx::Rect rc2 = rc;
  rc2.shrink(kResizeBorder);

  // Mouse in client area
  if (!rc.contains(pos)) {
    return Hit::Content;
  }
  // Resize edges
  if (!rc2.contains(pos) &&
      // macOS cannot start the resizing actions (just the window movement)
      System::instance()->hasCapability(Capabilities::CanStartWindowResize)) {
    if (pos.y < kResizeBorder) {
      if (pos.x < kResizeBorder)
        return Hit::TopLeft;
      if (pos.x > rc.x2() - kResizeBorder)
        return Hit::TopRight;
      return Hit::Top;
    }
    if (pos.y > rc.y2() - kResizeBorder) {
      if (pos.x < kResizeBorder)
        return Hit::BottomLeft;
      if (pos.x > rc.x2() - kResizeBorder)
        return Hit::BottomRight;
      return Hit::Bottom;
    }
    if (pos.x < rc.w / 2)
      return Hit::Left;
    return Hit::Right;
  }
  if (pos.y <= kTitleBarSize) {
    if (pos.x > rc.x2() - kButtonSize)
      return Hit::CloseButton;
    if (pos.x > rc.x2() - kButtonSize * 2)
      return Hit::MaximizeButton;
    if (pos.x > rc.x2() - kButtonSize * 3)
      return Hit::MinimizeButton;
    return Hit::TitleBar;
  }
  return Hit::Content;
}

void draw_button(Surface* surface, int x, Hit button, const Hit hit)
{
  Paint p;
  gfx::Rect box(x, 0, kButtonSize, kButtonSize);

  p.color(hit == button ? kTitleBarHigh : kTitleBarBase);
  p.style(Paint::Fill);
  surface->drawRect(box, p);

  p.color(gfx::rgba(25, 25, 50));
  p.style(Paint::Stroke);
  surface->drawRect(gfx::Rect(x, 0, 2, kButtonSize), p);

  // Draw icon
  box.shrink(11);
  box.inflate(1, 1);
  p.strokeWidth(1.5f);
  p.antialias(true);
  switch (button) {
    case Hit::MinimizeButton: surface->drawRect(gfx::Rect(box.x, box.y2() - 2, box.w, 1), p); break;
    case Hit::MaximizeButton: surface->drawRect(gfx::Rect(box), p); break;
    case Hit::CloseButton:    {
      gfx::Path path;
      path.moveTo(box.x, box.y);
      path.lineTo(box.x2(), box.y2());
      path.moveTo(box.x2(), box.y);
      path.lineTo(box.x, box.y2());
      surface->drawPath(path, p);
      break;
    }
    default: break;
  }
}

void draw_window(Window* window, const FontRef& font, const Hit hit)
{
  Surface* surface = window->surface();
  const gfx::Rect rc = surface->bounds();

  gfx::Rect rc2 = rc;
  Paint p;
  p.style(Paint::Fill);

  // Draw custom title bar area
  if (!window->isFullscreen()) {
    rc2.h = kTitleBarSize;

    p.color(hit == Hit::TitleBar ? kTitleBarHigh : kTitleBarBase);
    surface->drawRect(gfx::Rect(rc2).inflate(-kButtonSize * 3, 0), p);

    rc2.y += kTitleBarSize / 2 - 10;

    p.color(kTitleBarText);
    draw_text(surface, font, window->title(), rc2.center(), &p, TextAlign::Center);

    // Draw buttons
    draw_button(surface, rc.x2() - kButtonSize, Hit::CloseButton, hit);
    draw_button(surface, rc.x2() - kButtonSize * 2, Hit::MaximizeButton, hit);
    draw_button(surface, rc.x2() - kButtonSize * 3, Hit::MinimizeButton, hit);

    // Client area
    rc2 = rc;
    rc2.y += kTitleBarSize;
    rc2.h -= kTitleBarSize;
  }

  // Draw client area
  p.color(hit == Hit::Content ? kContentHigh : kContentBase);
  surface->drawRect(rc2, p);

  p.style(Paint::Style::Stroke);
  p.color(kContentEdge);
  surface->drawRect(rc2, p);

  p.style(Paint::Style::Fill);
  p.color(kContentText);
  draw_text(surface, font, "Content Rect", rc2.center(), &p, TextAlign::Center);

  if (window->isFullscreen()) {
    auto pos = rc2.center();
    pos.y += 24;
    draw_text(surface, font, "(F key or F11 to exit full screen)", pos, &p, TextAlign::Center);
  }

  if (window->isVisible()) {
    window->invalidateRegion(gfx::Region(rc));
    window->swapBuffers();
  }
  else
    window->setVisible(true);
}

bool update_hit(Window* window, const Event& ev, Hit& hit)
{
  Hit newHit = hit_test(window, ev.position());
  if (newHit != hit) {
    hit = newHit;
    return true;
  }
  return false;
}

WindowRef create_window()
{
  WindowSpec spec;
  spec.contentRect(gfx::Rect(32, 32, 400, 300));
  spec.titled(false);
  spec.borderless(true);

  WindowRef window = System::instance()->makeWindow(spec);
  window->setTitle("Custom Window");
  window->handleHitTest = hit_test;

  return window;
}

void handle_mouse_move(Window* window, const Hit hit)
{
  NativeCursor cursor = NativeCursor::Arrow;
  switch (hit) {
    case Hit::Content:     cursor = NativeCursor::Arrow; break;
    case Hit::TitleBar:    cursor = NativeCursor::Move; break;
    case Hit::TopLeft:     cursor = NativeCursor::SizeNW; break;
    case Hit::Top:         cursor = NativeCursor::SizeN; break;
    case Hit::TopRight:    cursor = NativeCursor::SizeNE; break;
    case Hit::Left:        cursor = NativeCursor::SizeW; break;
    case Hit::Right:       cursor = NativeCursor::SizeE; break;
    case Hit::BottomLeft:  cursor = NativeCursor::SizeSW; break;
    case Hit::Bottom:      cursor = NativeCursor::SizeS; break;
    case Hit::BottomRight: cursor = NativeCursor::SizeSE; break;
    default:               break;
  }
  window->setCursor(cursor);
}

bool handle_mouse_down(Window* window, const Event& ev, const Hit hit)
{
  NativeCursor cursor = NativeCursor::Arrow;
  WindowAction action = WindowAction::Move;
  switch (hit) {
    case Hit::Content:        return true;
    case Hit::TitleBar:       action = WindowAction::Move; break;
    case Hit::TopLeft:        action = WindowAction::ResizeFromTopLeft; break;
    case Hit::Top:            action = WindowAction::ResizeFromTop; break;
    case Hit::TopRight:       action = WindowAction::ResizeFromTopRight; break;
    case Hit::Left:           action = WindowAction::ResizeFromLeft; break;
    case Hit::Right:          action = WindowAction::ResizeFromRight; break;
    case Hit::BottomLeft:     action = WindowAction::ResizeFromBottomLeft; break;
    case Hit::Bottom:         action = WindowAction::ResizeFromBottom; break;
    case Hit::BottomRight:    action = WindowAction::ResizeFromBottomRight; break;
    case Hit::MinimizeButton: window->minimize(); return true;
    case Hit::MaximizeButton: window->maximize(); return true;
    case Hit::CloseButton:    return false;
    default:                  break;
  }
  window->performWindowAction(action, &ev);
  return true;
}

int app_main(int argc, char* argv[])
{
  SystemRef system = System::make();
  system->setAppMode(AppMode::GUI);

  FontRef font = FontMgr::Make()->defaultFont();
  WindowRef window = create_window();
  Hit hit = Hit::None; // Current area which the mouse cursor hits
  window->activate();

  system->handleWindowResize = [&](Window* w) { draw_window(w, font, hit); };
  system->finishLaunching();
  system->activateApp();

  EventQueue* queue = system->eventQueue();
  bool running = true;
  bool redraw = true;
  while (running) {
    if (redraw) {
      redraw = false;
      draw_window(window.get(), font, hit);
    }

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
          case kKeyF:
          case kKeyF11: window->setFullscreen(!window->isFullscreen()); break;
          default:      break;
        }
        break;

      case Event::MouseEnter:
      case Event::MouseMove:
        redraw = update_hit(window.get(), ev, hit);
        handle_mouse_move(window.get(), hit);
        break;

      case Event::MouseDown:
        redraw = update_hit(window.get(), ev, hit);
        if (!handle_mouse_down(window.get(), ev, hit))
          running = false;
        break;

      case Event::MouseLeave: redraw = update_hit(window.get(), ev, hit); break;

      default:                break;
    }
  }

  return 0;
}

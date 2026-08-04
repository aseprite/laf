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

class CustomWindow {
public:
  void create(const std::string& title, int w, int h, os::Window* parent = nullptr)
  {
    os::WindowSpec spec;

    if (parent) {
      spec.floating(true);
      spec.parent(parent);
      spec.minimizable(false);

      gfx::Rect rc = parent->frame();
      spec.frame(gfx::Rect(rc.x2() - w / 2, rc.y + rc.h / 2 - h / 2, w, h));
      spec.position(os::WindowSpec::Position::Frame);
    }
    else {
      spec.contentRect(gfx::Rect(0, 0, w, h));
      spec.position(os::WindowSpec::Position::Default);
    }

    m_nativeWindow = System::instance()->makeWindow(spec);
    m_nativeWindow->setTitle(title);
    m_nativeWindow->setUserData(this);

    redraw();
  }

  void close() { m_nativeWindow = nullptr; }

  void focus() { m_nativeWindow->activate(); }

  bool isVisible() const { return (m_nativeWindow && m_nativeWindow->isVisible()); }

  void redraw()
  {
    m_nativeWindow->makeCurrent();

    auto rc = m_nativeWindow->surface()->bounds();
    onRedraw(m_nativeWindow->surface(), rc);

    if (m_nativeWindow->isVisible()) {
      m_nativeWindow->invalidateRegion(gfx::Region(rc));
      m_nativeWindow->swapBuffers();
    }
    else {
      m_nativeWindow->setVisible(true);
    }
  }

  virtual bool isFloating() const { return false; }

  virtual bool handleEvent(os::Event& ev)
  {
    if (handle_toggle_gpu_key(ev)) {
      redraw();
      return true;
    }

    switch (ev.type()) {
      case os::Event::CloseWindow:  return false;

      case os::Event::ResizeWindow: redraw(); break;

      case os::Event::KeyDown:
        if (ev.scancode() == os::kKeyEsc)
          return false;
        else if (ev.scancode() == os::kKeyEnter || ev.scancode() == os::kKeyEnterPad)
          onEnterKey();
        break;

      default: break;
    }
    return true;
  }

protected:
  virtual void onRedraw(os::Surface* surface, const gfx::Rect& rc) {}
  virtual void onEnterKey() {}

  os::WindowRef nativeWindow() const { return m_nativeWindow; }

private:
  os::WindowRef m_nativeWindow;
};

class FloatingWindow : public CustomWindow {
public:
  void recreate(os::Window* parent) { create("Floating", 200, 200, parent); }

  bool isFloating() const override { return true; }

private:
  void onRedraw(os::Surface* surface, const gfx::Rect& rc) override
  {
    os::Paint paint;
    paint.color(gfx::rgba(200, 150, 150));
    surface->drawRect(rc, paint);
  }

  bool handleEvent(os::Event& ev) override
  {
    if (!CustomWindow::handleEvent(ev))
      close();
    return true;
  }
};

class MainWindow : public CustomWindow {
public:
  MainWindow() : m_font(FontMgr::Make()->defaultFont(12))
  {
    create("Main", 500, 400);
    createFloating();
  }

private:
  void onRedraw(os::Surface* surface, const gfx::Rect& rc) override
  {
    os::Paint p;
    p.color(gfx::rgba(150, 150, 200));
    surface->drawRect(rc, p);

    p.color(gfx::rgba(50, 50, 100));

    gfx::Point pos = rc.center();
    draw_text(surface,
              m_font,
              "Press ENTER key to hide/show the floating window",
              pos,
              &p,
              TextAlign::Center);

    pos.y += 24;
    draw_text(surface, m_font, "Press ESC to quit", pos, &p, TextAlign::Center);
  }

  void onEnterKey() override
  {
    if (m_floating.isVisible())
      m_floating.close();
    else {
      createFloating();
    }
  }

  void createFloating()
  {
    m_floating.recreate(nativeWindow().get());

    // Focus the main window (so the floating window is not focused by
    // default)
    focus();
  }

  FloatingWindow m_floating;
  FontRef m_font;
};

int app_main(int argc, char* argv[])
{
  SystemRef system = System::make();
  system->setAppMode(os::AppMode::GUI);
  system->handleWindowResize = [](os::Window* window) {
    window->userData<CustomWindow>()->redraw();
  };

  MainWindow mainWindow;

  system->finishLaunching();
  system->activateApp();

  os::EventQueue* queue = system->eventQueue();
  os::Event ev;
  bool done = false;
  while (!done) {
    queue->getEvent(ev);

    // Closing the app
    if (ev.type() == os::Event::CloseApp) {
      break;
    }
    // Each window handle the event
    if (ev.window()) {
      CustomWindow* win = ev.window()->userData<CustomWindow>();
      if (!win->handleEvent(ev))
        break;
    }
  }

  return 0;
}

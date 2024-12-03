// LAF OS Library
// Copyright (C) 2019-2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_WINDOW_SPEC_H_INCLUDED
#define OS_WINDOW_SPEC_H_INCLUDED
#pragma once

#include "gfx/rect.h"
#include "gfx/size.h"
#include "os/screen.h"

namespace os {

  class Window;

  class WindowSpec {
  public:
    // Position of the window by default
    enum class Position {
      Default,       // Default position selected by the OS (e.g. on Windows it is CW_USEDEFAULT)
      Frame,         // Position the window in the exact frame() coordinates
      ContentRect,   // Position the window leaving the client area origin in the exact contentRect() coordinates
      Center,
    };

    WindowSpec() {
    }

    WindowSpec(int width, int height, int scale = 1)
      : m_contentRect(0, 0, width, height)
      , m_scale(scale) {
    }

    Position position() const { return m_position; }
    bool titled() const { return m_titled; }
    bool borderless() const { return m_borderless; }
    bool closable() const { return m_closable; }
    bool maximizable() const { return m_maximizable; }
    bool minimizable() const { return m_minimizable; }
    bool resizable() const { return m_resizable; }
    bool floating() const { return m_floating; }
    bool transparent() const { return m_transparent; }
    bool modal() const { return m_modal; }

    // Parent window used for floating windows
    Window* parent() const { return m_parent; }

    void position(const Position p) { m_position = p; }
    void titled(const bool s) { m_titled = s; }
    void borderless(const bool s) { m_borderless = s; }
    void closable(const bool s) { m_closable = s; }
    void maximizable(const bool s) { m_maximizable = s; }
    void minimizable(const bool s) { m_minimizable = s; }
    void resizable(const bool s) { m_resizable = s; }
    void floating(const bool s) { m_floating = s; }
    void transparent(const bool s) { m_transparent = s; }
    void parent(Window* p) { m_parent = p; }
    void modal(const bool s) { m_modal = s; }

    const gfx::Rect& frame() const { return m_frame; }
    const gfx::Rect& contentRect() const { return m_contentRect; }
    int scale() const { return m_scale; }
    const ScreenRef& screen() const { return m_screen; }

    void frame(const gfx::Rect& frame) { m_frame = frame; }
    void contentRect(const gfx::Rect& contentRect) { m_contentRect = contentRect; }
    void scale(const int scale) { m_scale = scale; }
    void screen(const ScreenRef& screen) { m_screen = screen; }

  private:
    Position m_position = Position::Default;
    bool m_titled = true;
    bool m_borderless = false;
    bool m_closable = true;
    bool m_maximizable = true;
    bool m_minimizable = true;
    bool m_resizable = true;
    bool m_floating = false;
    bool m_transparent = false;
    bool m_modal = false;
    gfx::Rect m_frame;
    gfx::Rect m_contentRect;
    int m_scale = 1;
    ScreenRef m_screen;
    Window* m_parent = nullptr;
  };

} // namespace os

#endif

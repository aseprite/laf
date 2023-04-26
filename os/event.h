// LAF OS Library
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_EVENT_H_INCLUDED
#define OS_EVENT_H_INCLUDED
#pragma once

#include "base/paths.h"
#include "gfx/point.h"
#include "gfx/size.h"
#include "os/keys.h"
#include "os/pointer_type.h"
#include "os/window.h"

#include <functional>

#pragma push_macro("None")
#undef None // Undefine the X11 None macro

namespace os {

  class Event {
  public:
    enum Type {
      None,

      // macOS: When the Quit option is selected from the popup menu
      // of the app icon in the dock bar.
      CloseApp,

      // When the X is pressed in the current window.
      CloseWindow,

      // When the window is resized/maximized/restored (any time the
      // client area size of the window changes)
      ResizeWindow,

      // Some files are dropped in the window
      DropFiles,

      // Common mouse events
      MouseEnter,
      MouseLeave,
      MouseMove,
      MouseDown,
      MouseUp,
      MouseWheel,
      MouseDoubleClick,

      // A key is pressed (or is autorepeated if the key is kept pressed)
      KeyDown,

      // A key is released
      KeyUp,

      // Pinch gesture with fingers to zoom in/out
      TouchMagnify,
      Callback,
    };

    enum MouseButton {
      NoneButton,
      LeftButton,
      RightButton,
      MiddleButton,
      X1Button,
      X2Button,
    };

    Event() : m_type(None),
              m_window(nullptr),
              m_scancode(kKeyNil),
              m_modifiers(kKeyUninitializedModifier),
              m_unicodeChar(0),
              m_isDead(false),
              m_repeat(0),
              m_preciseWheel(false),
              m_pointerType(PointerType::Unknown),
              m_button(NoneButton),
              m_magnification(0.0f),
              m_pressure(0.0f) {
    }

    Type type() const { return m_type; }
    const WindowRef& window() const { return m_window; }
    const base::paths& files() const { return m_files; }
    // TODO Rename this to virtualKey(), which is the real
    // meaning. Then we need another kind of "scan code" with the
    // position in the keyboard, which might be useful to identify
    // keys by its position (e.g. WASD keys in other keyboard
    // layouts).
    KeyScancode scancode() const { return m_scancode; }
    KeyModifiers modifiers() const { return m_modifiers; }
    int unicodeChar() const { return m_unicodeChar; }
    bool isDeadKey() const { return m_isDead; }
    int repeat() const { return m_repeat; }
    gfx::Point position() const { return m_position; }
    gfx::Point wheelDelta() const { return m_wheelDelta; }
    gfx::PointF wheelDeltaF() const { return m_wheelDeltaF; }

    // We suppose that if we are receiving precise scrolling deltas,
    // it means that the user is using a touch-like surface (trackpad,
    // magic mouse scrolling, touch wacom tablet, etc.)
    bool preciseWheel() const { return m_preciseWheel; }

    PointerType pointerType() const { return m_pointerType; }
    MouseButton button() const { return m_button; }
    float magnification() const { return m_magnification; }
    float pressure() const { return m_pressure; }

    void setType(Type type) { m_type = type; }
    void setWindow(const WindowRef& window) { m_window = window; }
    void setFiles(const base::paths& files) { m_files = files; }
    void setCallback(std::function<void()>&& func) { m_callback = std::move(func); }

    void setScancode(KeyScancode scancode) { m_scancode = scancode; }
    void setModifiers(KeyModifiers modifiers) { m_modifiers = modifiers; }
    void setUnicodeChar(int unicodeChar) { m_unicodeChar = unicodeChar; }
    void setDeadKey(bool state) { m_isDead = state; }
    void setRepeat(int repeat) { m_repeat = repeat; }
    void setPosition(const gfx::Point& pos) { m_position = pos; }
    void setWheelDelta(const gfx::Point& delta) { m_wheelDelta = delta; }
    void setWheelDeltaF(const gfx::PointF& delta) { m_wheelDeltaF = delta; }
    void setPreciseWheel(bool precise) { m_preciseWheel = precise; }
    void setPointerType(PointerType pointerType) { m_pointerType = pointerType; }
    void setButton(MouseButton button) { m_button = button; }
    void setMagnification(float magnification) { m_magnification = magnification; }
    void setPressure(float pressure) { m_pressure = pressure; }

    void execCallback() { if (m_callback) m_callback(); }

  private:
    Type m_type;
    WindowRef m_window;
    base::paths m_files;
    std::function<void()> m_callback;
    KeyScancode m_scancode;
    KeyModifiers m_modifiers;
    int m_unicodeChar;
    bool m_isDead;
    int m_repeat; // repeat=0 means the first time the key is pressed
    gfx::Point m_position;
    gfx::Point m_wheelDelta;
    gfx::PointF m_wheelDeltaF;
    bool m_preciseWheel;
    PointerType m_pointerType;
    MouseButton m_button;

    // For TouchMagnify event
    float m_magnification;

    // Pressure of stylus used in mouse-like events
    float m_pressure;
  };

} // namespace os

#pragma pop_macro("None")

#endif

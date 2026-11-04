// LAF OS Library
// Copyright (C) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_WIN_IME_MANAGER_H_INCLUDED
#define OS_WIN_IME_MANAGER_H_INCLUDED

#include "gfx/point.h"

#include <windows.h>

namespace os {

class IMEManagerWin {
public:
  IMEManagerWin();
  bool textInput() const { return m_textInput; }
  void setTextInput(bool state) { m_textInput = state; }
  bool composing() const { return m_composing; }

  // Store caret in screen coordinates and cache client-relative position.
  void setCaretScreenPos(HWND hwnd, const gfx::Point& screenPos);

  // Re-apply Imm position using cached client caret.
  // When dragWindowRect is set (from WM_MOVING's lParam), compensate for
  // HWND lagging behind the drag rectangle so the IME tracks the preview.
  void updateImePosition(HWND hwnd, const RECT* dragWindowRect = nullptr) const;

  void onStartComposition(HWND hwnd);
  void onEndComposition();

  static IMEManagerWin* instance();

private:
  void cacheClientCaretPos(HWND hwnd);

  gfx::Point m_screenCaretPos;
  gfx::Point m_clientCaretPos;
  bool m_textInput;
  bool m_composing;
};

} // namespace os

#endif

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
  void setScreenCaretPos(const gfx::Point& pos) { m_screenCaretPos = pos; }

  void onStartComposition(HWND hwnd) const;

  static IMEManagerWin* instance();

private:
  gfx::Point m_screenCaretPos;
  bool m_textInput;
};

} // namespace os

#endif

// LAF OS Library
// Copyright (C) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ime_manager.h"

#include "base/debug.h"
#include "os/event_queue.h"

#include <windows.h>

// Must be included after windows.h
#include <imm.h>
#pragma comment(lib, "imm32.lib")

namespace os {

static IMEManagerWin g_imeManager;

IMEManagerWin* IMEManagerWin::instance()
{
  return &g_imeManager;
}

IMEManagerWin::IMEManagerWin()
{
  // Initialize the IME manager
  m_screenCaretPos = gfx::Point(0, 0);
  m_textInput = false;
}

void IMEManagerWin::onStartComposition(HWND hwnd) const
{
  // Set IME form position
  RECT windowRect;
  HIMC imc = ImmGetContext(hwnd);

  if (imc && GetWindowRect(hwnd, &windowRect)) {
    // Get caret relative position to the window
    POINT pos = {
      m_screenCaretPos.x - windowRect.left,
      m_screenCaretPos.y - windowRect.top,
    };
    // Set IME form position: just below the caret
    COMPOSITIONFORM cf = { 0 };
    cf.dwStyle = CFS_FORCE_POSITION;
    cf.ptCurrentPos = pos;
    ImmSetCompositionWindow(imc, &cf);
  }
}

} // namespace os

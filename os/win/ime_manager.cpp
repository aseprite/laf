// LAF OS Library
// Copyright (C) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "os/win/ime_manager.h"

#include <windows.h>

// Must be included after windows.h
#include <imm.h>

namespace os {

class ImmCtx {
public:
  ImmCtx(HWND hwnd) : m_hwnd(hwnd), m_imc(ImmGetContext(hwnd)) {}
  ~ImmCtx()
  {
    if (m_imc)
      ImmReleaseContext(m_hwnd, m_imc);
  }
  operator HIMC() { return m_imc; }

private:
  HWND m_hwnd;
  HIMC m_imc;
};

static IMEManagerWin g_imeManager;

IMEManagerWin* IMEManagerWin::instance()
{
  return &g_imeManager;
}

IMEManagerWin::IMEManagerWin()
{
  // Initialize the IME manager
  m_screenCaretPos = gfx::Point(0, 0);
  m_clientCaretPos = gfx::Point(0, 0);
  m_textInput = false;
  m_composing = false;
}

void IMEManagerWin::cacheClientCaretPos(HWND hwnd)
{
  RECT rc;
  GetClientRect(hwnd, &rc);
  ClientToScreen(hwnd, (LPPOINT)&rc);

  m_clientCaretPos = gfx::Point(m_screenCaretPos.x - rc.left, m_screenCaretPos.y - rc.top);
}

void IMEManagerWin::setCaretScreenPos(HWND hwnd, const gfx::Point& screenPos)
{
  m_screenCaretPos = screenPos;
  cacheClientCaretPos(hwnd);
}

void IMEManagerWin::updateImePosition(HWND hwnd, const RECT* dragWindowRect) const
{
  ImmCtx imc(hwnd);
  if (!imc) {
    return;
  }

  POINT pos = { m_clientCaretPos.x, m_clientCaretPos.y };

  // During WM_MOVING the HWND can lag behind the drag rectangle in
  // lParam. Imm maps client coords through the current HWND origin, so
  // offset by the drag delta to keep the IME on the preview position.
  if (dragWindowRect) {
    RECT curWindow;
    GetWindowRect(hwnd, &curWindow);
    pos.x += dragWindowRect->left - curWindow.left;
    pos.y += dragWindowRect->top - curWindow.top;
  }

  COMPOSITIONFORM cf = { 0 };
  cf.dwStyle = CFS_FORCE_POSITION;
  cf.ptCurrentPos = pos;
  ImmSetCompositionWindow(imc, &cf);

  // Candidate list is a separate top-level window; composition updates alone will leave it lagging.
  // CFS_EXCLUDE (SDL-style) anchors it near the caret without forcing an absolute candidate spot
  // that some IMEs reject at STARTCOMPOSITION. See:
  // https://learn.microsoft.com/en-us/windows/win32/api/imm/nf-imm-immsetcandidatewindow
  // https://learn.microsoft.com/en-us/windows/win32/api/imm/ns-imm-candidateform
  CANDIDATEFORM cand = { 0 };
  cand.dwIndex = 0;
  cand.dwStyle = CFS_EXCLUDE;
  cand.ptCurrentPos = pos;

  // Exclude the caret cell: height from the composition font when available.
  LONG caretTop = pos.y;
  LOGFONTW font = {};
  if (ImmGetCompositionFontW(imc, &font) && font.lfHeight != 0) {
    LONG fontHeight = (font.lfHeight < 0) ? (0 - font.lfHeight) : font.lfHeight;
    caretTop = pos.y - fontHeight;
  }

  cand.rcArea.left = pos.x;
  cand.rcArea.top = caretTop;
  cand.rcArea.right = pos.x + 1;
  cand.rcArea.bottom = pos.y + 1;

  ImmSetCandidateWindow(imc, &cand);
}

void IMEManagerWin::onStartComposition(HWND hwnd)
{
  m_composing = true;

  ImmCtx imc(hwnd);
  if (!imc) {
    return;
  }

  RECT rc;
  GetClientRect(hwnd, &rc);
  ClientToScreen(hwnd, (LPPOINT)&rc);

  // Get caret relative position to the window
  POINT pos = {
    m_screenCaretPos.x - rc.left,
    m_screenCaretPos.y - rc.top,
  };

  m_clientCaretPos = gfx::Point(pos.x, pos.y);

  // Set IME form position: just below the caret
  COMPOSITIONFORM cf = { 0 };
  cf.dwStyle = CFS_POINT;
  cf.ptCurrentPos = pos;

  ImmSetCompositionWindow(imc, &cf);
}

void IMEManagerWin::onEndComposition()
{
  m_composing = false;
}

} // namespace os

// LAF OS Library
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <queue>

#include <windows.h>

#include "os/win/event_queue.h"

#include "base/time.h"
#include "os/win/ime_manager.h"

namespace os {

void EventQueueWin::queueEvent(const Event& ev)
{
  m_events.push(ev);
}

void EventQueueWin::clearEvents()
{
  m_events.clear();
}

void EventQueueWin::getEvent(Event& ev, double timeout)
{
  const base::tick_t untilTick = base::current_tick() + timeout * 1000.0;
  MSG msg;

  ev.setWindow(nullptr);

  while (m_events.empty()) {
    BOOL res;

    if (timeout == kWithoutTimeout) {
      res = GetMessage(&msg, nullptr, 0, 0);
    }
    else {
      const base::tick_t now = base::current_tick();
      if (untilTick > now) {
        const base::tick_t msecs = (untilTick - now);
        MsgWaitForMultipleObjects(0,
                                  nullptr,
                                  FALSE,
                                  (DWORD)msecs, // Milliseconds to wait
                                  QS_ALLINPUT | QS_ALLPOSTMESSAGE);
      }
      res = PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
    }

    if (res) {
      // Avoid transforming WM_KEYDOWN/UP into WM_DEADCHAR/WM_CHAR
      // messages. Dead keys are converted manually in the
      // WM_KEYDOWN processing on our WindowWin<T> class.
      //
      // Unless we are processing a WM_KEYDOWN message with VK_PROCESSKEY,
      // which is used to process IME (Input Method Editor) messages.
      //
      // From MSDN TranslateMessage() documentation:
      //   "WM_KEYDOWN and WM_KEYUP combinations produce a WM_CHAR
      //   or WM_DEADCHAR message."
      // https://msdn.microsoft.com/en-us/library/windows/desktop/ms644955.aspx
      //
      // Keyboard Input documentation:
      //    "the IME sets the virtual key value to VK_PROCESSKEY after
      //    processing a key input message"
      // https://learn.microsoft.com/en-us/windows/win32/learnwin32/keyboard-input
      // https://learn.microsoft.com/en-us/windows/win32/api/imm/nf-imm-immgetvirtualkey
      if (msg.message == WM_KEYDOWN || msg.message == WM_KEYUP) {
#if LAF_WITH_IME
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_PROCESSKEY) {
          if (IMEManagerWin::instance()->textInput()) {
            // If we are in text input mode, we need to process the
            // WM_KEYDOWN message to IME.
            TranslateMessage(&msg);
          }
          else {
            // If we are not in text input mode, ignore it.
            msg.message = WM_NULL;
          }
        }
#endif
      }
      else {
        TranslateMessage(&msg);
      }
      DispatchMessage(&msg);
    }
    else if (timeout != kWithoutTimeout)
      break;
  }

  if (!m_events.try_pop(ev)) {
    ev.setType(Event::None);
  }
}

} // namespace os

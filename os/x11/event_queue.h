// LAF OS Library
// Copyright (C) 2021-present  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_X11_EVENT_QUEUE_INCLUDED
#define OS_X11_EVENT_QUEUE_INCLUDED
#pragma once

#include "base/concurrent_queue.h"
#include "os/event.h"
#include "os/event_queue.h"
#include "os/x11/x11.h"

#include <atomic>
#include <deque>

namespace os {

class WindowX11;

class EventQueueX11 : public EventQueue {
public:
  EventQueueX11();
  ~EventQueueX11();

  void queueEvent(const Event& ev) override;
  void getEvent(Event& ev, double timeout) override;
  void clearEvents() override;

  bool isEmpty() const { return m_events.empty(); }

private:
  void processX11Event(XEvent& event);

  base::concurrent_queue<Event> m_events;
  std::atomic<bool> m_sleeping;
};

using EventQueueImpl = EventQueueX11;

} // namespace os

#endif

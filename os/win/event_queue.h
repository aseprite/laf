// LAF OS Library
// Copyright (C) 2020-2021  Igara Studio S.A.
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_WIN_EVENT_QUEUE_INCLUDED
#define OS_WIN_EVENT_QUEUE_INCLUDED
#pragma once

#include "base/concurrent_queue.h"
#include "os/event.h"
#include "os/event_queue.h"

#include <queue>

namespace os {

class EventQueueWin : public EventQueue {
public:
  void queueEvent(const Event& ev) override;
  void getEvent(Event& ev, double timeout) override;
  void clearEvents();

private:
  base::concurrent_queue<Event> m_events;
};

using EventQueueImpl = EventQueueWin;

} // namespace os

#endif

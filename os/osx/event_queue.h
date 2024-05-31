// LAF OS Library
// Copyright (C) 2018-2021  Igara Studio S.A.
// Copyright (C) 2015-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_OSX_EVENT_QUEUE_INCLUDED
#define OS_OSX_EVENT_QUEUE_INCLUDED
#pragma once

#include "base/concurrent_queue.h"
#include "os/event.h"
#include "os/event_queue.h"

namespace os {

class EventQueueOSX : public EventQueue {
public:
  EventQueueOSX();

  void getEvent(Event& ev, double timeout) override;
  void queueEvent(const Event& ev) override;
  void clearEvents() override;

private:
  void wakeUpQueue();

  base::concurrent_queue<Event> m_events;
  mutable std::mutex m_mutex;
  bool m_sleeping;
};

using EventQueueImpl = EventQueueOSX;

} // namespace os

#endif

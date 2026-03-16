// LAF OS Library
// Copyright (c) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_IOS_EVENT_QUEUE_INCLUDED
#define OS_IOS_EVENT_QUEUE_INCLUDED
#pragma once

#include "os/event.h"
#include "os/event_queue.h"

#include <deque>
#include <mutex>

namespace os {

class EventQueueIOS : public EventQueue {
public:
  void getEvent(Event& ev, double timeout) override;
  void queueEvent(const Event& ev) override;
  void clearEvents() override;

private:
  std::deque<Event> m_events;
  mutable std::mutex m_mutex;
};

using EventQueueImpl = EventQueueIOS;

} // namespace os

#endif

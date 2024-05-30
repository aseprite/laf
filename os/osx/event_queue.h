// LAF OS Library
// Copyright (C) 2018-2024  Igara Studio S.A.
// Copyright (C) 2015-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_OSX_EVENT_QUEUE_INCLUDED
#define OS_OSX_EVENT_QUEUE_INCLUDED
#pragma once

#include "os/event.h"
#include "os/event_queue.h"

#include <map>
#include <mutex>
#include <cstdint>

namespace os {

class EventQueueOSX : public EventQueue {
public:
  EventQueueOSX();

  void getEvent(Event& ev, double timeout) override;
  void queueEvent(const Event& ev) override;
  void clearEvents() override;

private:
  void extractEvent(Event& ev, uint32_t eventId);

  mutable std::mutex m_mutex;
  std::map<uint32_t, os::Event> m_events;
  uint32_t m_nextEventId;
};

using EventQueueImpl = EventQueueOSX;

} // namespace os

#endif

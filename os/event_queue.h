// LAF OS Library
// Copyright (C) 2021  Igara Studio S.A.
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_EVENT_QUEUE_H_INCLUDED
#define OS_EVENT_QUEUE_H_INCLUDED
#pragma once

namespace os {

  class Event;

  class EventQueue {
  public:
    static constexpr const double kWithoutTimeout = -1.0;

    virtual ~EventQueue() { }

    // Wait for a new event. We can specify a timeout in seconds to
    // limit the time of wait for the next event.
    virtual void getEvent(Event& ev, double timeout = kWithoutTimeout) = 0;
    virtual void queueEvent(const Event& ev) = 0;
    virtual void clearEvents() = 0;

    // Deprecated old method. We should remove this line after some
    // releases. It's here to avoid calling getEvent(Event&, double)
    // even when we use a bool 2nd argument.
    void getEvent(Event& ev, bool) = delete;

    // On macOS we need the EventQueue before the creation of the
    // System. E.g. when we double-click a file an Event to open that
    // file is queued in application:openFile:, code which is executed
    // before the user's main() code.
    static EventQueue* instance();
  };

  inline void queue_event(const Event& ev) {
    EventQueue::instance()->queueEvent(ev);
  }

} // namespace os

#endif

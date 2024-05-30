// LAF OS Library
// Copyright (C) 2018-2024  Igara Studio S.A.
// Copyright (C) 2015-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

#include "os/osx/event_queue.h"

#define EV_TRACE(...)

namespace os {

EventQueueOSX::EventQueueOSX()
{
}

void EventQueueOSX::getEvent(Event& ev, double timeout)
{
  // This autoreleasepool is required to release all received NSEvent
  // objects (if this is not used, NSEvents are stored in memory and
  // NSWindow* references are kept).
  @autoreleasepool {
    // Calculate the date to wait messages if there are no messages in
    // the queue (we calculate this date here because the "timeout"
    // parameter depends on the current time, so we prefer to use it
    // ASAP inside the function).
    NSDate* untilDate;
    if (timeout != kWithoutTimeout) {
      untilDate = [[NSDate alloc] init]; // Equivalent to [NSDate now] on macOS 10.15+
      if (timeout > 0.0)
        untilDate = [untilDate dateByAddingTimeInterval:timeout];
    }
    else {
      untilDate = [NSDate distantFuture];
    }

    ev.setType(Event::None);
    ev.setWindow(nullptr);

    NSApplication* app = [NSApplication sharedApplication];
    if (!app)
      return;

    EV_TRACE("EV: Waiting for events\n");

    NSEvent* event;
    do {
      // Pump the whole queue of Cocoa events
      event = [app nextEventMatchingMask:NSEventMaskAny
                               untilDate:untilDate
                                  inMode:NSDefaultRunLoopMode
                                 dequeue:YES];

      if (event) {
        // Intercept <Control+Tab>, <Cmd+[>, and other keyboard
        // combinations, and send them directly to the main
        // NSView. Without this, the NSApplication intercepts the key
        // combination and use it to go to the next key view.
        if (event.type == NSEventTypeKeyDown &&
            app.keyWindow) {
          [app.keyWindow.contentView keyDown:event];
        }
        else if (event.type == NSEventTypeApplicationDefined) {
          const uint32_t eventId = [event data1];
          extractEvent(ev, eventId);
          return;
        }
        else {
          [app sendEvent:event];
        }
      }
    } while (event);

    EV_TRACE("EV: Timeout!");
  }
}

void EventQueueOSX::queueEvent(const Event& ev)
{
  const std::lock_guard lock(m_mutex);

  NSApplication* app = [NSApplication sharedApplication];
  if (!app)
    return;

  // Save the event for latter retrieval.
  const uint32_t eventId = ++m_nextEventId;
  m_events[eventId] = ev;
  [app postEvent:[NSEvent otherEventWithType:NSApplicationDefined
                                    location:NSZeroPoint
                               modifierFlags:0
                                   timestamp:0
                                windowNumber:0
                                     context:nullptr
                                     subtype:ev.type()
                                       data1:eventId
                                       data2:0]
       atStart:NO];
}

void EventQueueOSX::clearEvents()
{
  const std::lock_guard lock(m_mutex);
  m_events.clear();

  NSApplication* app = [NSApplication sharedApplication];
  if (!app)
    return;

  [app discardEventsMatchingMask:NSEventMaskAny
                     beforeEvent:[app currentEvent]];
}

void EventQueueOSX::extractEvent(Event& ev, const uint32_t eventId)
{
  const std::lock_guard lock(m_mutex);
  auto nh = m_events.extract(eventId);
  ev = nh.mapped();
}

} // namespace os

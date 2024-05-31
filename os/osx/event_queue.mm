// LAF OS Library
// Copyright (C) 2018-2021  Igara Studio S.A.
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
  : m_sleeping(false)
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

    NSEvent* event;
    do {
      // Pump the whole queue of Cocoa events
      event = [app nextEventMatchingMask:NSEventMaskAny
                               untilDate:[NSDate distantPast]
                                  inMode:NSDefaultRunLoopMode
                                 dequeue:YES];

    retry:
      if (event) {
        // Intercept <Control+Tab>, <Cmd+[>, and other keyboard
        // combinations, and send them directly to the main
        // NSView. Without this, the NSApplication intercepts the key
        // combination and use it to go to the next key view.
        if (event.type == NSEventTypeKeyDown &&
            app.keyWindow) {
          [app.keyWindow.contentView keyDown:event];
        }
        else {
          [app sendEvent:event];
        }
      }
    } while (event);

    m_mutex.lock();
    if (!m_events.try_pop(ev)) {
      if (timeout == kWithoutTimeout)
        EV_TRACE("EV: Waiting for events\n");

      // Wait until there is a Cocoa event in queue
      m_sleeping = true;
      m_mutex.unlock();
      event = [app nextEventMatchingMask:NSEventMaskAny
                               untilDate:untilDate
                                  inMode:NSDefaultRunLoopMode
                                 dequeue:YES];
      m_mutex.lock();
      m_sleeping = false;
      m_mutex.unlock();

      if (event) {
        EV_TRACE("EV: Event received!\n");
        goto retry;
      }
      else {
        EV_TRACE("EV: Timeout!");
      }
    }
    m_mutex.unlock();
  }
}

void EventQueueOSX::queueEvent(const Event& ev)
{
  const std::lock_guard lock(m_mutex);
  if (m_sleeping) {
    // Wake up the macOS event queue. This is necessary in case that we
    // change the display color profile from macOS settings: the
    // display surface is regenerated with the new color space, a
    // Event::ResizeDisplay event is enqueued and we have to start
    // processing macOS events again. If we don't wake up the events
    // queue, the window keeps with a black background and is not
    // re-painted until we receive some event from the OS, like a
    // mouse movement.
    wakeUpQueue();
    m_sleeping = false;
  }
  m_events.push(ev);
}

void EventQueueOSX::wakeUpQueue()
{
  EV_TRACE("EV: Force queue wake up!\n");

  NSApplication* app = [NSApplication sharedApplication];
  if (!app)
    return;

  [app postEvent:[NSEvent otherEventWithType:NSApplicationDefined
                                    location:NSZeroPoint
                               modifierFlags:0
                                   timestamp:0
                                windowNumber:0
                                     context:nullptr
                                     subtype:0
                                       data1:0
                                       data2:0]
         atStart:NO];
}

void EventQueueOSX::clearEvents()
{
  m_events.clear();
}

} // namespace os

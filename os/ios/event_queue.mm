// LAF OS Library
// Copyright (c) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#import <UIKit/UIKit.h>

#include "os/ios/event_queue.h"

namespace os {

void EventQueueIOS::getEvent(Event& ev, double timeout)
{
  ev.setType(Event::None);

  {
    std::lock_guard lock(m_mutex);
    if (!m_events.empty()) {
      ev = m_events.front();
      m_events.pop_front();
      return;
    }
  }

  @autoreleasepool {
    NSDate* limitDate;
    if (timeout > 0.0)
      limitDate = [NSDate dateWithTimeIntervalSinceNow:timeout];
    else
      limitDate = [NSDate distantFuture];

    [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                             beforeDate:limitDate];
  }

  {
    std::lock_guard lock(m_mutex);
    if (!m_events.empty()) {
      ev = m_events.front();
      m_events.pop_front();
    }
  }
}

void EventQueueIOS::queueEvent(const Event& ev)
{
  std::lock_guard lock(m_mutex);
  m_events.push_back(ev);
}

void EventQueueIOS::clearEvents()
{
  std::lock_guard lock(m_mutex);
  m_events.clear();
}

} // namespace os

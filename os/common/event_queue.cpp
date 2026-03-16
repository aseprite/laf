// LAF OS Library
// Copyright (C) 2021  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#if LAF_WINDOWS
  #include "os/win/event_queue.h"
#elif LAF_IOS
  #include "os/ios/event_queue.h"
#elif LAF_MACOS
  #include "os/osx/event_queue.h"
#elif LAF_LINUX
  #include "os/x11/event_queue.h"
#endif

namespace os {

EventQueueImpl g_queue;

EventQueue* EventQueue::instance()
{
  return &g_queue;
}

} // namespace os

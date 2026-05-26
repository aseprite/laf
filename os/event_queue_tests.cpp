// LAF OS Library
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <gtest/gtest.h>

#include "os/event.h"
#include "os/event_queue.h"
#include "os/system.h"

#include <chrono>
#include <condition_variable>
#include <thread>

using namespace std::chrono_literals;

bool done = false;
std::mutex mutex;
std::condition_variable cv;

std::thread queue_callback()
{
  return std::thread([] {
    // Wait a little of time so we call getEvent() from the main thread.
    std::this_thread::sleep_for(15ms);

    os::Event ev;
    ev.setType(os::Event::Callback);
    ev.setCallback([] { done = true; });
    os::queue_event(ev);
  });
}

std::thread kill_me_in_two_seconds()
{
  return std::thread([] {
    std::unique_lock lock(mutex);
    if (cv.wait_for(lock, 2s) == std::cv_status::timeout) {
      EXPECT_FALSE(true) << "Event queue is stuck";
      std::exit(1);
    }
  });
}

TEST(EventQueue, WakeupForCallback)
{
  auto system = os::System::make();

  std::thread a = kill_me_in_two_seconds();
  std::thread b = queue_callback();

  while (!done) {
    os::Event ev;
    os::EventQueue::instance()->getEvent(ev);
    switch (ev.type()) {
      case os::Event::Callback: ev.execCallback(); break;
      default:
        // Do nothing
        break;
    }
  }

  cv.notify_one();
  EXPECT_TRUE(done);

  b.join();
  a.join();
}

int app_main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

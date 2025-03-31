// LAF Base Library
// Copyright (C) 2019-2022  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_THREAD_POOL_H_INCLUDED
#define BASE_THREAD_POOL_H_INCLUDED
#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace base {

class thread_pool {
public:
  class work {
    friend class thread_pool;

  public:
    work(std::function<void()>&& func) { m_func = std::move(func); }

  private:
    std::function<void()> m_func = nullptr;
  };

  typedef std::unique_ptr<work> work_ptr;

  thread_pool(const size_t n);
  ~thread_pool();

  const work* execute(std::function<void()>&& func);

  // Removes the specified work from the queue if possible. Returns true if it
  // was able to do so, or false otherwise.
  bool try_pop(const work* w);

  // Waits until the queue is empty.
  void wait_all();

private:
  // Joins all threads without waiting the queue to be processed.
  void join_all();

  // Called for each worker thread.
  void worker();

  bool m_running;
  std::vector<std::thread> m_threads;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::condition_variable m_cvWait;
  std::deque<work_ptr> m_work;
  int m_doingWork;
};

} // namespace base

#endif

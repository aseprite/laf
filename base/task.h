// LAF Base Library
// Copyright (C) 2019  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_TASK_H_INCLUDED
#define BASE_TASK_H_INCLUDED
#pragma once

#include "base/debug.h"
#include "base/thread_pool.h"

#include <atomic>
#include <functional>

namespace base {

class task;
class thread_pool;

class task_token {
  friend class task;

public:
  task_token() : m_canceled(false), m_progress(0.0f), m_progress_min(0.0f), m_progress_max(1.0f) {}

  bool canceled() const { return m_canceled; }
  float progress() const { return m_progress; }

  void cancel() { m_canceled = true; }
  void set_progress(float p)
  {
    ASSERT(p >= 0.0f && p <= 1.0f);
    m_progress = m_progress_min + p * (m_progress_max - m_progress_min);
  }
  void set_progress_range(float min, float max)
  {
    m_progress_min = min;
    m_progress_max = max;
  }

private:
  task_token(const task_token& token)
  {
    m_canceled.store(token.m_canceled);
    m_progress.store(token.m_progress);
    m_progress_min = token.m_progress_min;
    m_progress_max = token.m_progress_max;
    m_work = token.m_work;
  }

  void reset()
  {
    m_canceled = false;
    m_progress = 0.0f;
  }

  std::atomic<bool> m_canceled;
  std::atomic<float> m_progress;
  float m_progress_min, m_progress_max;
  const thread_pool::work* m_work = nullptr;
};

class task {
public:
  enum class state {
    READY,    // task is created an ready to be started
    ENQUEUED, // task is enqueued in the thread pool waiting for execution
    RUNNING,  // task is being executed
    FINISHED  // task finished execution by either success, error, or cancellation
  };

  typedef std::function<void(task_token&)> func_t;
  // Type for the "finished" callback function. A const reference to a copy of
  // the token is received to allow safe destruction of the task inside the
  // callback, and to let the caller know that the token cannot be modified
  // (because there is no point in updating the token after task finalization)
  typedef std::function<void(const task_token&)> finfunc_t;

  task();
  ~task();

  void on_execute(func_t&& f) { m_execute = std::move(f); }
  // Sets a callback that will be called after logical finalization of the task.
  // Useful for signaling task finalization instead of using polling techniques
  // to know if the task finished. Also, it can be used to destroy the task
  // safely.
  void on_finished(finfunc_t&& f) { m_finished = std::move(f); }

  task_token& start(thread_pool& pool);
  bool try_pop(thread_pool& pool);

  bool running() const { return m_state == state::RUNNING; }

  // Returns true when the task is enqueued in the thread pool's work queue,
  // and false when the task is actually being executed.
  bool enqueued() const { return m_state == state::ENQUEUED; }

  // Returns true when the task is completed (whether it was
  // canceled or not). If this is true, it's safe to delete the task
  // instance (it will not be used anymore by any other background
  // thread).
  bool completed() const { return m_state == state::FINISHED; }

private:
  void in_worker_thread();
  void call_finished();

  std::atomic<state> m_state;
  task_token m_token;
  func_t m_execute;
  finfunc_t m_finished = nullptr;
};

} // namespace base

#endif

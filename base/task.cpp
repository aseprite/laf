// LAF Base Library
// Copyright (C) 2019  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "base/task.h"

#include "base/debug.h"
#include "base/log.h"

namespace base {

task::task() : m_state(state::READY)
{
}

task::~task()
{
  // The task must not be running when we are destroying it.
  ASSERT(m_state != state::RUNNING);
}

task_token& task::start(thread_pool& pool)
{
  // Cannot start the task if it's already running or enqueued
  ASSERT(m_state != state::RUNNING && m_state != state::ENQUEUED);

  m_state = state::ENQUEUED;
  m_token.reset();

  m_token.m_work = pool.execute([this] { in_worker_thread(); });
  return m_token;
}

bool task::try_pop(thread_pool& pool)
{
  bool popped = pool.try_pop(m_token.m_work);
  if (popped) {
    m_token.m_canceled = true;
    // The task is not waiting for execution any more, we can safely execute the
    // finished callback. Note that at this point the task remains in
    // ENQUEUED state, this can be used to let the caller know that the task was
    // never run.
    call_finished();
  }

  return popped;
}

// Executes the "finished" callback only if it was set.
void task::call_finished()
{
  if (m_finished) {
    try {
      task_token token = m_token;
      m_finished(token);
    }
    catch (const std::exception& ex) {
      LOG(ERROR, "Exception executing 'finished' callback: %s\n", ex.what());
    }
  }
}

void task::in_worker_thread()
{
  m_state = state::RUNNING;
  try {
    if (!m_token.canceled())
      m_execute(m_token);
  }
  catch (const std::exception& ex) {
    LOG(FATAL, "Exception running task: %s\n", ex.what());
  }

  m_state = state::FINISHED;

  // The task finished execution, now we can call the finished callback safely
  // without worrying if the task is destroyed there.
  call_finished();
}

} // namespace base

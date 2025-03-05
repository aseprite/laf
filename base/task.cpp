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

  pool.execute([this] { in_worker_thread(); });
  return m_token;
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

  if (m_finished)
    m_finished(m_token);
}

} // namespace base

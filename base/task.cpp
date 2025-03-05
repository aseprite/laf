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

task::task() : m_running(false), m_completed(false)
{
}

task::~task()
{
  // The task must not be running when we are destroying it.
  ASSERT(!m_running);

  // m_completed can be false in this case if the task was never
  // started (i.e. the user never called task::start()).
  // ASSERT(m_completed);
}

task_token& task::start(thread_pool& pool)
{
  // Cannot start the task if it's already running
  ASSERT(!m_running);

  // Reset flags for a running task
  m_running = true;
  m_enqueued = true;
  m_completed = false;
  m_token.reset();

  pool.execute([this] { in_worker_thread(); });
  return m_token;
}

void task::in_worker_thread()
{
  m_enqueued = false;
  try {
    if (!m_token.canceled())
      m_execute(m_token);
  }
  catch (const std::exception& ex) {
    LOG(FATAL, "Exception running task: %s\n", ex.what());
  }

  m_running = false;

  // This must be the latest statement in the worker thread (see
  // task::complete() comment)
  m_completed = true;
  if (m_finished)
    m_finished(m_token);
}

} // namespace base

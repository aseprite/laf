// LAF Base Library
// Copyright (C) 2019-2021  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/thread.h"

#if LAF_WINDOWS
  #include <windows.h>
  #include <process.h>
#else
  #include <pthread.h>     // Use pthread library in Unix-like systems

  #include <unistd.h>
  #include <sys/time.h>
#endif

namespace {

#if LAF_WINDOWS

  DWORD WINAPI win32_thread_proxy(LPVOID data)
  {
    base::thread::details::thread_proxy(data);
    return 0;
  }

#else

  void* pthread_thread_proxy(void* data)
  {
    base::thread::details::thread_proxy(data);
    return NULL;
  }

#endif

}

namespace base {

thread::thread()
  : m_native_handle((native_handle_type)0)
#if LAF_WINDOWS
  , m_native_id((native_id_type)0)
#endif
{
}

thread::~thread()
{
  if (joinable()) {
#if LAF_WINDOWS
    ::CloseHandle(m_native_handle);
#endif  
    detach();
  }
}

bool thread::joinable() const
{
  return m_native_handle != (native_handle_type)0;
}

void thread::join()
{
  if (joinable()) {
#if LAF_WINDOWS
    ::WaitForSingleObject(m_native_handle, INFINITE);
    ::CloseHandle(m_native_handle);
#else
    ::pthread_join((pthread_t)m_native_handle, NULL);
#endif
    detach();
  }
}

void thread::detach()
{
  if (joinable()) {
    m_native_handle = (native_handle_type)NULL;
  }
}

thread::native_id_type thread::native_id() const
{
#if LAF_WINDOWS

  return m_native_id;

#else

  return (thread::native_id_type)m_native_handle;

#endif
}

void thread::launch_thread(func_wrapper* f)
{
  m_native_handle = (native_handle_type)0;

#if LAF_WINDOWS

  static_assert(sizeof(DWORD) == sizeof(native_id_type),
                "native_id_type must match DWORD size on Windows");

  m_native_handle = ::CreateThread(NULL, 0, win32_thread_proxy, (LPVOID)f,
                                   CREATE_SUSPENDED, (LPDWORD)&m_native_id);
  ResumeThread(m_native_handle);

#else

  pthread_t thread;
  if (::pthread_create(&thread, NULL, pthread_thread_proxy, f) == 0)
    m_native_handle = (void*)thread;

#endif
}

void thread::details::thread_proxy(void* data)
{
  func_wrapper* f = reinterpret_cast<func_wrapper*>(data);

  // Call operator() of func_wrapper class (this is a virtual method).
  (*f)();

  // Delete the data (it was created in the thread() ctor).
  delete f;
}

void this_thread::yield()
{
#if LAF_WINDOWS

  ::Sleep(0);

#elif defined(HAVE_SCHED_YIELD) && defined(_POSIX_PRIORITY_SCHEDULING)

  sched_yield();

#else

  timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  select(0, nullptr, nullptr, nullptr, &timeout);

#endif
}

void this_thread::sleep_for(double seconds)
{
#if LAF_WINDOWS

  ::Sleep(DWORD(seconds * 1000.0));

#else

  usleep(seconds * 1000000.0);

#endif
}

thread::native_id_type this_thread::native_id()
{
#if LAF_WINDOWS

  return (thread::native_id_type)GetCurrentThreadId();

#else

  return (thread::native_id_type)pthread_self();

#endif
}

} // namespace base

// LAF Base Library
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#ifdef LAF_WINDOWS
  #include <windows.h>
#endif

#include "base/log.h"

#include "base/debug.h"
#include "base/fstream_path.h"

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

namespace {

// Default log level is error, which means that we'll log regular
// errors and fatal errors.
std::atomic<LogLevel> log_level(LogLevel::ERROR);
std::mutex log_mutex;
std::ofstream log_stream;
std::ostream* log_ostream = &std::cerr;
std::string log_filename;

} // anonymous namespace

void base::set_log_filename(const char* filename)
{
  if (log_stream.is_open()) {
    log_stream.close();
    log_ostream = &std::cerr;
  }

  if (filename) {
    log_filename = filename;
    log_stream.open(FSTREAM_PATH(log_filename));
    log_ostream = &log_stream;
  }
  else {
    log_filename = std::string();
  }
}

void base::set_log_level(const LogLevel level)
{
  log_level = level;
}

LogLevel base::get_log_level()
{
  return log_level;
}

static void LOGva(const char* format, va_list ap)
{
  va_list apTmp;
  va_copy(apTmp, ap);
  const int size = std::vsnprintf(nullptr, 0, format, apTmp);
  va_end(apTmp);
  if (size < 1)
    return; // Nothing to log

  std::vector<char> buf(size + 1);
  std::vsnprintf(buf.data(), buf.size(), format, ap);

  {
    const std::lock_guard lock(log_mutex);
    ASSERT(log_ostream);
    log_ostream->write(buf.data(), size);
    log_ostream->flush();
  }

#ifdef _DEBUG
  #ifdef LAF_WINDOWS
  ::OutputDebugStringA(buf.data());
  #else
  fputs(buf.data(), stderr);
  fflush(stderr);
  #endif
#endif
}

void LOG(const char* format, ...)
{
  ASSERT(format);
  if (!format || log_level < INFO)
    return;

  va_list ap;
  va_start(ap, format);
  LOGva(format, ap);
  va_end(ap);
}

void LOG(const LogLevel level, const char* format, ...)
{
  ASSERT(format);
  if (!format || log_level < level)
    return;

  va_list ap;
  va_start(ap, format);
  LOGva(format, ap);
  va_end(ap);
}

// LAF Base Library
// Copyright (c) 2023-2024 Igara Studio S.A.
// Copyright (c) 2015-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_PROCESS_H_INCLUDED
#define BASE_PROCESS_H_INCLUDED
#pragma once

#include "base/ints.h"

#include <string>

namespace base {

  typedef uint32_t pid;

  pid get_current_process_id();

  std::string get_process_name(pid pid);

  bool is_process_running(pid pid, std::string currentProcessName);

  // Declaration to avoid errors during testing of Github actions
  // TO DO: remove function after the implementation of PR aseprite/aseprite#4266
  bool is_process_running(pid pid);

} // namespace base

#endif

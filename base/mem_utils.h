// LAF Base Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_MEM_UTILS_H_INCLUDED
#define BASE_MEM_UTILS_H_INCLUDED
#pragma once

#include <cstring>
#include <string>

namespace base {

  template<typename T>
  T copy_reinterpret_cast(const void* ptr) {
    T tmp;
    std::memcpy(&tmp, ptr, sizeof(T));
    return tmp;
  }

  std::string get_pretty_memory_size(std::size_t memsize);

} // namespace base

#endif

// LAF Base Library
// Copyright (c) 2023-2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_UUID_H_INCLUDED
#define BASE_UUID_H_INCLUDED
#pragma once

#include "base/ints.h"

#include <cstring>

// Uncomment if you want to debug the generation of the UUID, and the
// its string conversion.
// #define LAF_BASE_TRACE_UUID 1
#if LAF_BASE_TRACE_UUID
  #include "base/convert_to.h"
  #include "base/string.h"
#endif

namespace base {

// A universally unique identifier.
class Uuid {
public:
  // Null UUID
  static const Uuid Null;

  enum { HashSize = 36 };

  static Uuid Generate();

  uint8_t operator[](int i) const { return m_data[i]; }
  bool operator==(const Uuid& b) const { return std::memcmp(m_data, b.m_data, 16) == 0; }
  bool operator!=(const Uuid& b) const { return !operator==(b); }

  const uint8_t* bytes() const { return &m_data[0]; }
  uint8_t* bytes() { return &m_data[0]; }

private:
  uint8_t m_data[16] = {};
};

} // namespace base

#endif

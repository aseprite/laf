// LAF Base Library
// Copyright (C) 2024-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_ENUM_FLAGS_H_INCLUDED
#define BASE_ENUM_FLAGS_H_INCLUDED
#pragma once

#include <atomic>
#include <type_traits>

// When C++0x was being redacted, I was expecting that the final C++11
// was going to introduce something similar to the C# [Flags]
// attribute, but that never happened. I tried some approaches in the
// past even to emulate C++11 enums in the C++03 era (enclosing the
// values in its own struct/namespace):
//
//   https://github.com/dacap/vaca/blob/main/vaca/Enum.h
//
// But now that we have "class enum" to define the type and values in
// its own scope/namespace, here is a straightforward approach: we
// just define the "enum class" and then use a macro to define all
// required operators:
//
//   enum class Name { A=1, B=2, C=4 };
//   LAF_ENUM_FLAGS(Name);
//
// There are libraries that do something similar like enum-flags:
//
//   https://github.com/grisumbras/enum-flags
//
#define LAF_ENUM_FLAGS(T)                                                                          \
  constexpr inline T operator~(const T a)                                                          \
  {                                                                                                \
    using U = std::underlying_type_t<T>;                                                           \
    return static_cast<T>(~static_cast<U>(a));                                                     \
  }                                                                                                \
                                                                                                   \
  constexpr inline T operator|(const T a, const T b)                                               \
  {                                                                                                \
    using U = std::underlying_type_t<T>;                                                           \
    return static_cast<T>(static_cast<U>(a) | static_cast<U>(b));                                  \
  }                                                                                                \
                                                                                                   \
  constexpr inline T operator&(const T a, const T b)                                               \
  {                                                                                                \
    using U = std::underlying_type_t<T>;                                                           \
    return static_cast<T>(static_cast<U>(a) & static_cast<U>(b));                                  \
  }                                                                                                \
                                                                                                   \
  constexpr inline T operator^(const T a, const T b)                                               \
  {                                                                                                \
    using U = std::underlying_type_t<T>;                                                           \
    return static_cast<T>(static_cast<U>(a) ^ static_cast<U>(b));                                  \
  }                                                                                                \
                                                                                                   \
  constexpr inline T& operator|=(T& a, const T b)                                                  \
  {                                                                                                \
    a = a | b;                                                                                     \
    return a;                                                                                      \
  }                                                                                                \
                                                                                                   \
  constexpr inline T& operator&=(T& a, const T b)                                                  \
  {                                                                                                \
    a = a & b;                                                                                     \
    return a;                                                                                      \
  }                                                                                                \
                                                                                                   \
  constexpr inline T& operator^=(T& a, const T b)                                                  \
  {                                                                                                \
    a = a ^ b;                                                                                     \
    return a;                                                                                      \
  }                                                                                                \
                                                                                                   \
  inline std::atomic<T>& operator|=(std::atomic<T>& a, const T b)                                  \
  {                                                                                                \
    a = a | b;                                                                                     \
    return a;                                                                                      \
  }                                                                                                \
                                                                                                   \
  inline std::atomic<T>& operator&=(std::atomic<T>& a, const T b)                                  \
  {                                                                                                \
    a = a & b;                                                                                     \
    return a;                                                                                      \
  }                                                                                                \
                                                                                                   \
  inline std::atomic<T>& operator^=(std::atomic<T>& a, const T b)                                  \
  {                                                                                                \
    a = a ^ b;                                                                                     \
    return a;                                                                                      \
  }

#endif

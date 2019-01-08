// LAF Base Library
// Copyright (c) 2019 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_GCD_H_INCLUDED
#define BASE_GCD_H_INCLUDED
#pragma once

namespace base {

  template<typename T>
  T gcd(T a, T b) {
    a = (a < 0 ? -a: a);
    b = (b < 0 ? -b: b);
    if (a > 1 && b > 1) {
      if (a == b)
        return a;
      else {
        while (a != b) {
          if (a > b) {
            a = a - b;
          }
          else if (a < b) {
            b = b - a;
          }
          if (a == b) {
            return a;
          }
        }
      }
    }
    return T(1);
  }

} // namespace base

#endif

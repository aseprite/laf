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
    T output = 1;
    if (a > 1 && b > 1) {
        if (a == b)
            output = 1;
        else {
            while (a != b) {
                if (a > b) {
                    a = a - b;
                }
                else if (a < b) {
                    b = b - a;
                }
                if (a == b) {
                    output = a;
                }
            }
        }
    }
    return output;
  }

} // namespace base

#endif

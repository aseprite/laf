// LAF Base Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_SHA1_H_INCLUDED
#define BASE_SHA1_H_INCLUDED
#pragma once

#include <vector>
#include <string>

#include "base/ints.h"

extern "C" struct SHA1Context;

namespace base {

  class Sha1 {
  public:
    enum { HashSize = 20 };

    Sha1();
    explicit Sha1(const std::vector<uint8_t>& digest);

    // Calculates the SHA1 of the given file or string.
    static Sha1 calculateFromFile(const std::string& fileName);
    static Sha1 calculateFromString(const std::string& text);

    bool operator==(const Sha1& other) const;
    bool operator!=(const Sha1& other) const;

    uint8_t operator[](int index) const {
      return m_digest[index];
    }

    const uint8_t *digest() const {
      return m_digest.data();
    }

    size_t size() const {
      return m_digest.size();
    }

  private:
    std::vector<uint8_t> m_digest;
  };

} // namespace base

#endif  // BASE_SHA1_H_INCLUDED

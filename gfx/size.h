// LAF Gfx Library
// Copyright (C) 2020-present  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef GFX_SIZE_H_INCLUDED
#define GFX_SIZE_H_INCLUDED
#pragma once

#include <algorithm>

namespace gfx {

template<typename T>
class PointT;
template<typename T>
class BorderT;

// A 2D size.
template<typename T>
class SizeT {
public:
  T w, h;

  constexpr SizeT() : w(0), h(0) {}

  constexpr SizeT(const T& w, const T& h) : w(w), h(h) {}

  constexpr SizeT(const SizeT& size) : w(size.w), h(size.h) {}

  template<typename U>
  constexpr explicit SizeT(const SizeT<U>& size)
    : w(static_cast<T>(size.w))
    , h(static_cast<T>(size.h))
  {
  }

  constexpr explicit SizeT(const PointT<T>& point) : w(point.x), h(point.y) {}

  [[nodiscard]]
  SizeT createUnion(const SizeT& sz) const
  {
    return SizeT(std::max(w, sz.w), std::max(h, sz.h));
  }

  [[nodiscard]]
  SizeT createIntersection(const SizeT& sz) const
  {
    return SizeT(std::min(w, sz.w), std::min(h, sz.h));
  }

  SizeT& operator=(const SizeT& sz)
  {
    w = sz.w;
    h = sz.h;
    return *this;
  }

  const SizeT& operator+=(const SizeT& sz)
  {
    w += sz.w;
    h += sz.h;
    return *this;
  }

  const SizeT& operator-=(const SizeT& sz)
  {
    w -= sz.w;
    h -= sz.h;
    return *this;
  }

  const SizeT& operator+=(const BorderT<T>& br)
  {
    w += br.width();
    h += br.height();
    return *this;
  }

  const SizeT& operator-=(const BorderT<T>& br)
  {
    w -= br.width();
    h -= br.height();
    return *this;
  }

  const SizeT& operator+=(const T& value)
  {
    w += value;
    h += value;
    return *this;
  }

  const SizeT& operator-=(const T& value)
  {
    w -= value;
    h -= value;
    return *this;
  }

  const SizeT& operator*=(const T& value)
  {
    w *= value;
    h *= value;
    return *this;
  }

  const SizeT& operator/=(const T& value)
  {
    w /= value;
    h /= value;
    return *this;
  }

  const SizeT& operator|=(const SizeT& sz) { return *this = createUnion(sz); }

  const SizeT& operator&=(const SizeT& sz) { return *this = createIntersection(sz); }

  [[nodiscard]]
  SizeT operator+(const SizeT& sz) const
  {
    return SizeT(w + sz.w, h + sz.h);
  }

  [[nodiscard]]
  SizeT operator-(const SizeT& sz) const
  {
    return SizeT(w - sz.w, h - sz.h);
  }

  [[nodiscard]]
  SizeT operator+(const BorderT<T>& br) const
  {
    return SizeT(w + br.width(), h + br.height());
  }

  [[nodiscard]]
  SizeT operator-(const BorderT<T>& br) const
  {
    return SizeT(w - br.width(), h - br.height());
  }

  [[nodiscard]]
  SizeT operator+(const T& value) const
  {
    return SizeT(w + value, h + value);
  }

  [[nodiscard]]
  SizeT operator-(const T& value) const
  {
    return SizeT(w - value, h - value);
  }

  [[nodiscard]]
  SizeT operator*(const T& value) const
  {
    return SizeT(w * value, h * value);
  }

  [[nodiscard]]
  SizeT operator/(const T& value) const
  {
    return SizeT(w / value, h / value);
  }

  [[nodiscard]]
  SizeT operator-() const
  {
    return SizeT(-w, -h);
  }

  [[nodiscard]]
  SizeT operator|(const SizeT& other) const
  {
    return createUnion(other);
  }

  [[nodiscard]]
  SizeT operator&(const SizeT& other) const
  {
    return createIntersection(other);
  }

  [[nodiscard]]
  bool operator==(const SizeT& sz) const
  {
    return w == sz.w && h == sz.h;
  }

  [[nodiscard]]
  bool operator!=(const SizeT& sz) const
  {
    return w != sz.w || h != sz.h;
  }
};

using Size = SizeT<int>;
using SizeF = SizeT<float>;

} // namespace gfx

#ifdef _DEBUG
  #include "gfx/size_io.h"
#endif

#endif

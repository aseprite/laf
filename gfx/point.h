// LAF Gfx Library
// Copyright (C) 2020-present  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef GFX_POINT_H_INCLUDED
#define GFX_POINT_H_INCLUDED
#pragma once

namespace gfx {

template<typename T>
class SizeT;

// A 2D coordinate in the screen.
template<typename T>
class PointT {
public:
  T x, y;

  constexpr PointT() : x(0), y(0) {}

  constexpr PointT(const T& x, const T& y) : x(x), y(y) {}

  constexpr PointT(const PointT& point) : x(point.x), y(point.y) {}

  template<typename U>
  constexpr explicit PointT(const PointT<U>& point)
    : x(static_cast<T>(point.x))
    , y(static_cast<T>(point.y))
  {
  }

  template<typename U>
  constexpr explicit PointT(const SizeT<U>& size) : x(size.w)
                                                  , y(size.h)
  {
  }

  template<typename U>
  PointT& operator=(const PointT<U>& pt)
  {
    x = pt.x;
    y = pt.y;
    return *this;
  }

  template<typename U>
  const PointT& operator+=(const PointT<U>& pt)
  {
    x += pt.x;
    y += pt.y;
    return *this;
  }

  template<typename U>
  const PointT& operator-=(const PointT<U>& pt)
  {
    x -= pt.x;
    y -= pt.y;
    return *this;
  }

  const PointT& operator+=(const T& value)
  {
    x += value;
    y += value;
    return *this;
  }

  const PointT& operator-=(const T& value)
  {
    x -= value;
    y -= value;
    return *this;
  }

  const PointT& operator*=(const T& value)
  {
    x *= value;
    y *= value;
    return *this;
  }

  const PointT& operator/=(const T& value)
  {
    x /= value;
    y /= value;
    return *this;
  }

  template<typename U>
  [[nodiscard]]
  PointT operator+(const PointT<U>& pt) const
  {
    return PointT(x + pt.x, y + pt.y);
  }

  template<typename U>
  [[nodiscard]]
  PointT operator-(const PointT<U>& pt) const
  {
    return PointT(x - pt.x, y - pt.y);
  }

  [[nodiscard]]
  PointT operator+(const T& value) const
  {
    return PointT(x + value, y + value);
  }

  [[nodiscard]]
  PointT operator-(const T& value) const
  {
    return PointT(x - value, y - value);
  }

  [[nodiscard]]
  PointT operator*(const T& value) const
  {
    return PointT(x * value, y * value);
  }

  [[nodiscard]]
  PointT operator/(const T& value) const
  {
    return PointT(x / value, y / value);
  }

  [[nodiscard]]
  PointT operator-() const
  {
    return PointT(-x, -y);
  }

  [[nodiscard]]
  bool operator==(const PointT& pt) const
  {
    return x == pt.x && y == pt.y;
  }

  [[nodiscard]]
  bool operator!=(const PointT& pt) const
  {
    return x != pt.x || y != pt.y;
  }
};

using Point = PointT<int>;
using PointF = PointT<float>;

} // namespace gfx

#ifdef _DEBUG
  #include "gfx/point_io.h"
#endif

#endif

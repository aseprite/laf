// LAF Gfx Library
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef GFX_RECT_H_INCLUDED
#define GFX_RECT_H_INCLUDED
#pragma once

#include <cmath>

namespace gfx {

template<typename T> class PointT;
template<typename T> class SizeT;
template<typename T> class BorderT;

// A rectangle.
template<typename T>
class RectT {
public:
  T x, y, w, h;

  T x2() const { return x+w; }
  T y2() const { return y+h; }

  // Creates a new empty rectangle with the origin in 0,0.
  RectT() : x(0), y(0), w(0), h(0) {
  }

  // Creates a new rectangle with the specified size with the origin in 0,0.
  RectT(const T& w, const T& h) :
    x(0), y(0),
    w(w), h(h) {
  }

  // Creates a new rectangle with the specified size with the origin in 0,0.
  explicit RectT(const SizeT<T>& size) :
    x(0), y(0),
    w(size.w), h(size.h) {
  }

  RectT(const RectT<T>& rect) :
    x(rect.x), y(rect.y),
    w(rect.w), h(rect.h) {
  }

  template<typename U>
  RectT(const RectT<U>& rect) :
    x(static_cast<T>(rect.x)), y(static_cast<T>(rect.y)),
    w(static_cast<T>(rect.w)), h(static_cast<T>(rect.h)) {
  }

  RectT(const PointT<T>& point, const SizeT<T>& size) :
    x(point.x), y(point.y),
    w(size.w), h(size.h) {
  }

  // Creates a new rectangle with the origin in point1 and size
  // equal to point2-point1.
  //
  // If a coordinate of point1 is greater than point2, the coordinates
  // are swapped. The resulting rectangle will be:
  //
  // x = min(point1.x, point2.x)
  // y = min(point1.y, point2.y)
  // w = max(point1.x, point2.x) - x
  // h = max(point1.x, point2.x) - y
  //
  // See that point2 isn't included in the rectangle, it's like the
  // point returned by point2() member function.
  RectT(const PointT<T>& point1, const PointT<T>& point2) {
    PointT<T> leftTop = point1;
    PointT<T> rightBottom = point2;
    T t;

    if (leftTop.x > rightBottom.x) {
      t = leftTop.x;
      leftTop.x = rightBottom.x;
      rightBottom.x = t;
    }

    if (leftTop.y > rightBottom.y) {
      t = leftTop.y;
      leftTop.y = rightBottom.y;
      rightBottom.y = t;
    }

    this->x = leftTop.x;
    this->y = leftTop.y;
    this->w = rightBottom.x - leftTop.x;
    this->h = rightBottom.y - leftTop.y;
  }

  RectT(const T& x, const T& y, const T& w, const T& h) : x(x), y(y), w(w), h(h) {
  }

  // Verifies if the width and/or height of the rectangle are less or
  // equal than zero.
  bool isEmpty() const {
    return (w <= 0 || h <= 0);
  }

  // Returns the middle point of the rectangle (x+w/2, y+h/2).
  PointT<T> center() const {
    return PointT<T>(x+w/2, y+h/2);
  }

  // Returns the point in the upper-left corner (that is inside the
  // rectangle).
  PointT<T> origin() const {
    return PointT<T>(x, y);
  }

  // Returns point in the lower-right corner that is outside the
  // rectangle (x+w, y+h).
  PointT<T> point2() const {
    return PointT<T>(x+w, y+h);
  }

  SizeT<T> size() const {
    return SizeT<T>(w, h);
  }

  RectT& setOrigin(const PointT<T>& pt) {
    x = pt.x;
    y = pt.y;
    return *this;
  }

  RectT& setSize(const SizeT<T>& sz) {
    w = sz.w;
    h = sz.h;
    return *this;
  }

  // Moves the rectangle origin in the specified delta.
  RectT& offset(const T& dx, const T& dy) {
    x += dx;
    y += dy;
    return *this;
  }

  template<typename U>
  RectT& offset(const PointT<U>& delta) {
    x += delta.x;
    y += delta.y;
    return *this;
  }

  RectT& inflate(const T& delta) {
    w += delta;
    h += delta;
    return *this;
  }

  RectT& inflate(const T& dw, const T& dh) {
    w += dw;
    h += dh;
    return *this;
  }

  RectT& inflate(const SizeT<T>& delta) {
    w += delta.w;
    h += delta.h;
    return *this;
  }

  RectT& enlarge(const T& unit) {
    x -= unit;
    y -= unit;
    w += unit*2;
    h += unit*2;
    return *this;
  }

  RectT& enlarge(const BorderT<T>& br) {
    x -= br.left();
    y -= br.top();
    w += br.left() + br.right();
    h += br.top() + br.bottom();
    return *this;
  }

  RectT& enlargeXW(const T& unit) {
    x -= unit;
    w += unit*2;
    return *this;
  }

  RectT& enlargeYH(const T& unit) {
    y -= unit;
    h += unit*2;
    return *this;
  }

  RectT& shrink(const T& unit) {
    x += unit;
    y += unit;
    w -= unit*2;
    h -= unit*2;
    return *this;
  }

  RectT& shrink(const BorderT<T>& br) {
    x += br.left();
    y += br.top();
    w -= br.left() + br.right();
    h -= br.top() + br.bottom();
    return *this;
  }

  // Adjusts the x/y floating-point coordinates to the left most/top
  // most near integer respectively (instead of rounding towards
  // zero). Basically applies the std::floor() function to the origin
  // of the rectangle.
  //
  // Generally only useful to convert a gfx::RectF to a gfx::Rect as
  // an alternative to gfx::Rect(gfx::RectF(...)).
  //
  // Example with floor():
  //
  //   gfx::RectF boundsF(-0.25, -0.75, 1, 2);
  //   gfx::Rect bounds = boundsF.floor();
  //   ASSERT(bounds == gfx::Rect(-1, -1, 1, 2));
  //
  // Without floor(), using the Rect(RectF()) ctor converting floating
  // point to integer directly:
  //
  //   gfx::RectF boundsF(-0.25, -0.75, 1, 2);
  //   gfx::Rect bounds = boundsF;
  //   ASSERT(bounds == gfx::Rect(0, 0, 1, 2));
  //
  RectT& floor() {
    x = std::floor(x);
    y = std::floor(y);
    return *this;
  }

  // Returns true if this rectangle encloses the pt point.
  bool contains(const PointT<T>& pt) const {
    return
      pt.x >= x && pt.x < x+w &&
      pt.y >= y && pt.y < y+h;
  }

  bool contains(const T& u, const T& v) const {
    return
      u >= x && u < x+w &&
      v >= y && v < y+h;
  }

  // Returns true if this rectangle entirely contains the rc rectangle.
  bool contains(const RectT& rc) const {
    if (isEmpty() || rc.isEmpty())
      return false;

    return
      rc.x >= x && rc.x+rc.w <= x+w &&
      rc.y >= y && rc.y+rc.h <= y+h;
  }

  // Returns true if the intersection between this rectangle with rc
  // rectangle is not empty.
  bool intersects(const RectT& rc) const {
    if (isEmpty() || rc.isEmpty())
      return false;

    return
      rc.x < x+w && rc.x+rc.w > x &&
      rc.y < y+h && rc.y+rc.h > y;
  }

  // Returns the union rectangle between this and rc rectangle.
  RectT createUnion(const RectT& rc) const {
    if (isEmpty())
      return rc;
    if (rc.isEmpty())
      return *this;
    return RectT(PointT<T>(x < rc.x ? x: rc.x,
                           y < rc.y ? y: rc.y),
                 PointT<T>(x+w > rc.x+rc.w ? x+w: rc.x+rc.w,
                           y+h > rc.y+rc.h ? y+h: rc.y+rc.h));
  }

  // Returns the intersection rectangle between this and rc rectangles.
  RectT createIntersection(const RectT& rc) const {
    if (intersects(rc))
      return RectT(PointT<T>(x > rc.x ? x: rc.x,
                             y > rc.y ? y: rc.y),
                   PointT<T>(x+w < rc.x+rc.w ? x+w: rc.x+rc.w,
                             y+h < rc.y+rc.h ? y+h: rc.y+rc.h));
    return RectT();
  }

  const RectT& operator+=(const BorderT<T>& br) {
    enlarge(br);
    return *this;
  }

  const RectT& operator-=(const BorderT<T>& br) {
    shrink(br);
    return *this;
  }

  RectT& operator*=(const T factor) {
    x *= factor;
    y *= factor;
    w *= factor;
    h *= factor;
    return *this;
  }

  RectT& operator/=(const T factor) {
    x /= factor;
    y /= factor;
    w /= factor;
    h /= factor;
    return *this;
  }

  RectT& operator*=(const SizeT<T>& size) {
    x *= size.w;
    y *= size.h;
    w *= size.w;
    h *= size.h;
    return *this;
  }

  RectT& operator/=(const SizeT<T>& size) {
    x /= size.w;
    y /= size.h;
    w /= size.w;
    h /= size.h;
    return *this;
  }

  const RectT& operator|=(const RectT& rc) {
    return *this = createUnion(rc);
  }

  const RectT& operator&=(const RectT& rc) {
    return *this = createIntersection(rc);
  }

  RectT operator+(const BorderT<T>& br) const {
    return RectT(*this).enlarge(br);
  }

  RectT operator-(const BorderT<T>& br) const {
    return RectT(*this).shrink(br);
  }

  RectT operator|(const RectT& other) const {
    return createUnion(other);
  }

  RectT operator&(const RectT& other) const {
    return createIntersection(other);
  }

  RectT operator*(const T factor) const {
    return RectT(x*factor, y*factor,
                 w*factor, h*factor);
  }

  RectT operator*(const SizeT<T>& size) const {
    return RectT(x*size.w, y*size.h,
                 w*size.w, h*size.h);
  }

  RectT operator/(const T scale) const {
    return RectT(x/scale, y/scale,
                 w/scale, h/scale);
  }

  RectT operator/(const SizeT<T>& size) const {
    return RectT(x/size.w, y/size.h,
                 w/size.w, h/size.h);
  }

  bool operator==(const RectT& rc) const {
    return
      x == rc.x && w == rc.w &&
      y == rc.y && h == rc.h;
  }

  bool operator!=(const RectT& rc) const {
    return
      x != rc.x || w != rc.w ||
      y != rc.y || h != rc.h;
  }

  RectT& fitIn(const RectT& bounds) {
    if (w < h) {
      w = w * bounds.h / h;
      x = bounds.x + bounds.w/2 - w/2;
      y = bounds.y;
      h = bounds.h;
    }
    else {
      h = h * bounds.w / w;
      y = bounds.y + bounds.h/2 - h/2;
      x = bounds.x;
      w = bounds.w;
    }
    return *this;
  }

  // Slices vertically this Rect along the provided px coordinate.
  // This rect is the right slice and the returned rect is the left slice.
  RectT sliceV(T px) {
    if (px < x)
      return RectT<T>();

    if (px > x2()) {
      auto leftSlice = *this;
      x = y = w = h = 0;
      return leftSlice;
    }

    gfx::RectT<T> leftSlice = RectT<T>(x, y, px - x, h);

    w = x2() - px;
    x = px;
    return leftSlice;
  }

  // Slices horizontally this Rect along the provided py coordinate.
  // This rect is the bottom slice and the returned rect is the top slice.
  RectT sliceH(T py) {
    if (py < y)
      return RectT<T>();

    if (py > y2()) {
      auto topSlice = *this;
      x = y = w = h = 0;
      return topSlice;
    }

    auto topSlice = RectT<T>(x, y, w, py - y);

    h = y2() - py;
    y = py;
    return topSlice;
  }

  // Slices this rect in nine pieces and returns all the rects in the slices
  // output array. The center rect defines the relative coordinates where the
  // cuts are going to be made:
  //
  //     this (x, y, w=23, h=7)       slices output
  //     +---------------------+      +--------+-----+------+
  //     | center (9,2,w=7,h=3)|      |   [0]  | [1] |  [2] |
  //     |        +-----+      |      +--------+-----+------+
  //     |        |     |      |  =>  |   [3]  | [4] |  [5] |
  //     |        +-----+      |      +--------+-----+------+
  //     |                     |      |   [6]  | [7] |  [8] |
  //     +---------------------+      +--------+-----+------+
  //
  // Note that this method doesn't modify this rect.
  void nineSlice(RectT<T> center, RectT<T> slices[9]) const {
    RectT<T> remaining = *this;
    gfx::RectT<T> left = remaining.sliceV(x + center.x);
    RectT<T> middle = remaining.sliceV(x + center.x2());
    RectT<T> right = remaining;

    slices[0] = left.sliceH(y + center.y);
    slices[1] = middle.sliceH(y + center.y);
    slices[2] = right.sliceH(y + center.y);

    slices[3] = left.sliceH(y + center.y2());
    slices[4] = middle.sliceH(y + center.y2());
    slices[5] = right.sliceH(y + center.y2());

    slices[6] = left;
    slices[7] = middle;
    slices[8] = right;
  }

};

using Rect = RectT<int>;
using RectF = RectT<float>;

} // namespace gfx

#ifdef _DEBUG
#include "gfx/rect_io.h"
#endif

#endif

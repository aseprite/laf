// LAF Gfx Library
// Copyright (c) 2020-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef GFX_PATH_NONE_H_INCLUDED
#define GFX_PATH_NONE_H_INCLUDED
#pragma once

#include "gfx/point.h"
#include "gfx/rect.h"

namespace gfx {

class Matrix;

class Path {
public:
  Path() {}
  Path& reset() { return *this; }
  Path& rewind() { return *this; }
  bool isEmpty() const { return true; }
  Path& moveTo(float x, float y) { return *this; }
  Path& moveTo(const Point& p) { return *this; }
  Path& lineTo(float x, float y) { return *this; }
  Path& lineTo(const Point& p) { return *this; }
  Path& cubicTo(float dx1, float dy1, float dx2, float dy2, float dx3, float dy3) { return *this; }
  Path& oval(const Rect& rc) { return *this; }
  Path& rect(const Rect& rc) { return *this; }
  Path& roundedRect(const Rect& rc, float rx, float ry) { return *this; }
  Path& close() { return *this; }
  void offset(float dx, float dy) {}
  void transform(const Matrix& matrix) {}
  Path snapshot(const Matrix& matrix) const { return {}; }
  RectF bounds() const { return RectF(); }
};

} // namespace gfx

#endif

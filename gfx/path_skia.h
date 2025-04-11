// LAF Gfx Library
// Copyright (c) 2020-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef GFX_PATH_SKIA_H_INCLUDED
#define GFX_PATH_SKIA_H_INCLUDED
#pragma once

#include "gfx/matrix.h"
#include "gfx/point.h"
#include "gfx/rect.h"

#include "include/core/SkPath.h"
#include "include/core/SkPathBuilder.h"

namespace gfx {

// Simple wrapper for SkPath
class Path {
public:
  Path() {}

  Path& reset()
  {
    m_modified = true;
    m_skPathBuilder.reset();
    return *this;
  }

  bool isEmpty() const
  {
    if (!m_modified)
      return m_skPath.isEmpty();
    return m_skPathBuilder.countPoints() == 0;
  }

  Path& moveTo(float x, float y)
  {
    m_modified = true;
    m_skPathBuilder.moveTo(x, y);
    return *this;
  }

  Path& moveTo(const Point& p)
  {
    m_modified = true;
    m_skPathBuilder.moveTo(p.x, p.y);
    return *this;
  }

  Path& lineTo(float x, float y)
  {
    m_modified = true;
    m_skPathBuilder.lineTo(x, y);
    return *this;
  }

  Path& lineTo(const Point& p)
  {
    m_modified = true;
    m_skPathBuilder.lineTo(p.x, p.y);
    return *this;
  }

  Path& cubicTo(float dx1, float dy1, float dx2, float dy2, float dx3, float dy3)
  {
    m_modified = true;
    m_skPathBuilder.cubicTo(dx1, dy1, dx2, dy2, dx3, dy3);
    return *this;
  }

  Path& oval(const Rect& rc)
  {
    m_modified = true;
    m_skPathBuilder.addOval(SkRect::MakeXYWH(rc.x, rc.y, rc.w, rc.h));
    return *this;
  }

  Path& rect(const Rect& rc)
  {
    m_modified = true;
    m_skPathBuilder.addRect(SkRect::MakeXYWH(rc.x, rc.y, rc.w, rc.h));
    return *this;
  }

  Path& roundedRect(const Rect& rc, float rx, float ry)
  {
    m_modified = true;
    m_skPathBuilder.addRRect(
      SkRRect::MakeRectXY(SkRect::MakeXYWH(rc.x + 0.5, rc.y + 0.5, rc.w, rc.h), rx, ry));
    return *this;
  }

  Path& close()
  {
    m_modified = true;
    m_skPathBuilder.close();
    return *this;
  }

  void offset(float dx, float dy)
  {
    m_modified = true;
    m_skPathBuilder.offset(dx, dy);
  }

  void transform(const Matrix& matrix)
  {
    m_modified = true;
    m_skPathBuilder.transform(matrix.skMatrix());
  }

  Path snapshot(const Matrix& matrix) const
  {
    Path dst;
    dst.m_skPath = m_skPathBuilder.snapshot(&matrix.skMatrix());
    dst.m_modified = false;
    return dst;
  }

  RectF bounds() const
  {
    if (isEmpty())
      return {};

    if (!m_modified) {
      auto rc = m_skPath.computeTightBounds();
      return RectF(rc.x(), rc.y(), rc.width(), rc.height());
    }

    auto rc = m_skPathBuilder.computeTightBounds();
    if (rc)
      return RectF(rc->x(), rc->y(), rc->width(), rc->height());
    return {};
  }

  const SkPath& skPath() const
  {
    if (m_modified) {
      m_skPath = m_skPathBuilder.snapshot();
      m_modified = false;
    }
    return m_skPath;
  }

private:
  SkPathBuilder m_skPathBuilder;
  mutable SkPath m_skPath;
  mutable bool m_modified = true;
};

} // namespace gfx

#endif

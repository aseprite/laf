// LAF Gfx Library
// Copyright (C) 2022-present  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef GFX_REGION_WIN_H_INCLUDED
#define GFX_REGION_WIN_H_INCLUDED
#pragma once

#include "gfx/point.h"
#include "gfx/rect.h"

#include <vector>

#ifndef _WINDOWS_
using LAF_LPRECT = void*;
using LAF_HRGN = void*;
using LAF_LPRGNDATA = void*;
#else
using LAF_LPRECT = LPRECT;
using LAF_HRGN = HRGN;
using LAF_LPRGNDATA = LPRGNDATA;
#endif

namespace gfx {

template<typename T>
class PointT;

class Region;

namespace details {

class RegionIterator {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = Rect;
  using difference_type = std::ptrdiff_t;
  using pointer = Rect*;
  using reference = Rect&;

  RegionIterator() {}
  RegionIterator(LAF_LPRECT prect) : m_prect(prect) {}
  RegionIterator(const RegionIterator& o) : m_prect(o.m_prect) {}
  RegionIterator& operator=(const RegionIterator& o)
  {
    m_prect = o.m_prect;
    return *this;
  }
  RegionIterator& operator++();
  bool operator==(const RegionIterator& o) const { return (m_prect == o.m_prect); }
  bool operator!=(const RegionIterator& o) const { return (m_prect != o.m_prect); }
  reference operator*();

private:
  LAF_LPRECT m_prect;
  gfx::Rect m_rect;
};

} // namespace details

class Region {
public:
  enum Overlap { Out, In, Part };

  using iterator = details::RegionIterator;
  using const_iterator = details::RegionIterator;

  Region();
  Region(const Region& copy);
  explicit Region(const Rect& rect);
  Region& operator=(const Rect& rect);
  Region& operator=(const Region& copy);
  ~Region();

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;

  bool isEmpty() const;
  bool isRect() const;
  bool isComplex() const;

  std::size_t size() const;

  Rect bounds() const;

  void clear();

  void offset(int dx, int dy);
  void offset(const PointT<int>& delta) { offset(delta.x, delta.y); }

  Region& createIntersection(const Region& a, const Region& b);
  Region& createUnion(const Region& a, const Region& b);
  Region& createSubtraction(const Region& a, const Region& b);

  bool contains(const PointT<int>& pt) const;
  Overlap contains(const Rect& rect) const;

  Region& operator+=(const Region& b) { return createUnion(*this, b); }
  Region& operator|=(const Region& b) { return createUnion(*this, b); }
  Region& operator&=(const Region& b) { return createIntersection(*this, b); }
  Region& operator-=(const Region& b) { return createSubtraction(*this, b); }

private:
  void resetData() const;
  void fillData() const;

  LAF_HRGN m_hrgn = nullptr;
  mutable LAF_LPRGNDATA m_data = nullptr;
};

} // namespace gfx

#endif

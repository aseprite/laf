// LAF Gfx Library
// Copyright (C) 2022 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <windows.h>

#include "gfx/region.h"

#include <cassert>

namespace gfx {

Region::Region() : m_hrgn(CreateRectRgn(0, 0, 0, 0))
{
}

Region::Region(const Region& copy) : m_hrgn(CreateRectRgn(0, 0, 0, 0))
{
  CombineRgn(m_hrgn, copy.m_hrgn, nullptr, RGN_COPY);
}

Region::Region(const Rect& rect) : m_hrgn(CreateRectRgn(rect.x, rect.y, rect.x2(), rect.y2()))
{
}

Region& Region::operator=(const Rect& rect)
{
  resetData();
  SetRectRgn(m_hrgn, rect.x, rect.y, rect.x2(), rect.y2());
  return *this;
}

Region& Region::operator=(const Region& copy)
{
  resetData();
  CombineRgn(m_hrgn, copy.m_hrgn, nullptr, RGN_COPY);
  return *this;
}

Region::~Region()
{
  resetData();
  if (m_hrgn)
    DeleteObject(m_hrgn);
}

Region::iterator Region::begin()
{
  fillData();
  return iterator(LPRECT(m_data->Buffer));
}

Region::iterator Region::end()
{
  fillData();
  return iterator(LPRECT(m_data->Buffer) + m_data->rdh.nCount);
}

Region::const_iterator Region::begin() const
{
  fillData();
  return const_iterator(LPRECT(m_data->Buffer));
}

Region::const_iterator Region::end() const
{
  fillData();
  return const_iterator(LPRECT(m_data->Buffer) + m_data->rdh.nCount);
}

bool Region::isEmpty() const
{
  RECT rc;
  return GetRgnBox(m_hrgn, &rc) == NULLREGION;
}

bool Region::isRect() const
{
  RECT rc;
  return GetRgnBox(m_hrgn, &rc) == SIMPLEREGION;
}

bool Region::isComplex() const
{
  RECT rc;
  return GetRgnBox(m_hrgn, &rc) == COMPLEXREGION;
}

Rect Region::bounds() const
{
  RECT rc = { 0, 0, 0, 0 };
  int res = GetRgnBox(m_hrgn, &rc);
  return gfx::Rect(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
}

std::size_t Region::size() const
{
  fillData();
  return m_data->rdh.nCount;
}

void Region::clear()
{
  resetData();
  SetRectRgn(m_hrgn, 0, 0, 0, 0);
}

void Region::offset(int dx, int dy)
{
  resetData();
  OffsetRgn(m_hrgn, dx, dy);
}

Region& Region::createIntersection(const Region& a, const Region& b)
{
  resetData();
  CombineRgn(m_hrgn, a.m_hrgn, b.m_hrgn, RGN_AND);
  return *this;
}

Region& Region::createUnion(const Region& a, const Region& b)
{
  resetData();
  CombineRgn(m_hrgn, a.m_hrgn, b.m_hrgn, RGN_OR);
  return *this;
}

Region& Region::createSubtraction(const Region& a, const Region& b)
{
  resetData();
  CombineRgn(m_hrgn, a.m_hrgn, b.m_hrgn, RGN_DIFF);
  return *this;
}

bool Region::contains(const PointT<int>& pt) const
{
  return PtInRegion(m_hrgn, pt.x, pt.y) ? true : false;
}

Region::Overlap Region::contains(const Rect& rect) const
{
  RECT rc = { rect.x, rect.y, rect.x2(), rect.y2() };
  if (RectInRegion(m_hrgn, &rc)) {
    Region rectRgn(rect);
    Region tmp;
    int res = CombineRgn(tmp.m_hrgn, rectRgn.m_hrgn, m_hrgn, RGN_AND);

    if (res == NULLREGION) {
      assert(false); // Impossible state? RectInRegion() said true above
      return Out;
    }
    if (EqualRgn(tmp.m_hrgn, rectRgn.m_hrgn))
      return In;
    return Part;
  }
  return Out;
}

void Region::resetData() const
{
  if (m_data) {
    delete[] m_data;
    m_data = nullptr;
  }
}

void Region::fillData() const
{
  if (m_data)
    return;

  int n = GetRegionData(m_hrgn, 0, nullptr);
  m_data = (LPRGNDATA)(new char[sizeof(RGNDATAHEADER) + n]);
  m_data->rdh.dwSize = sizeof(RGNDATAHEADER);
  if (n > 0)
    GetRegionData(m_hrgn, n, m_data);
}

details::RegionIterator& details::RegionIterator::operator++()
{
  ++m_prect;
  return *this;
}

details::RegionIterator::reference details::RegionIterator::operator*()
{
  m_rect.x = m_prect->left;
  m_rect.y = m_prect->top;
  m_rect.w = m_prect->right - m_prect->left;
  m_rect.h = m_prect->bottom - m_prect->top;
  return m_rect;
}

} // namespace gfx

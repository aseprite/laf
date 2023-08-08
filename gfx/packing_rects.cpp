// LAF Gfx Library
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/packing_rects.h"

#include "gfx/region.h"
#include "gfx/size.h"

namespace gfx {

void PackingRects::add(const Size& sz)
{
  m_rects.push_back(Rect(sz));
}

void PackingRects::add(const Rect& rc)
{
  m_rects.push_back(rc);
}

Size PackingRects::bestFit(base::task_token& token,
                           const int fixedWidth,
                           const int fixedHeight)
{
  Size size(fixedWidth, fixedHeight);

  // Nothing to do, the size is already specified
  if (fixedWidth > 0 && fixedHeight > 0)
    return size;

  // Calculate the amount of pixels that we need, the texture cannot
  // be smaller than that.
  int neededArea = 0;
  for (const auto& rc : m_rects) {
    neededArea += rc.w * rc.h;
    size |= rc.size();
  }

  const int w0 = std::max(size.w, 1);
  const int h0 = std::max(size.h, 1);
  int w = w0;
  int h = h0;
  int z = 0;
  bool fit = false;
  while (!token.canceled()) {
    if (w*h >= neededArea) {
      Size sizeCandidate = Size(w + 2 * m_borderPadding,
                                h + 2 * m_borderPadding);
      fit = pack(sizeCandidate, token);
      if (fit) {
        size = sizeCandidate;
        break;
      }
    }

    if (fixedWidth == 0 && fixedHeight == 0) {
      if ((++z) & 1)
        w += w0;
      else
        h += h0;
    }
    else if (fixedWidth == 0) {
      w += w0;
    }
    else {
      h += h0;
    }
  }

  return size;
}

static bool by_area(const Rect* a, const Rect* b) {
  return a->w*a->h > b->w*b->h;
}

bool PackingRects::pack(const Size& size,
                        base::task_token& token)
{
  m_bounds = Rect(size).shrink(m_borderPadding);

  // We cannot sort m_rects because we want to
  std::vector<Rect*> rectPtrs(m_rects.size());
  int i = 0;
  for (auto& rc : m_rects)
    rectPtrs[i++] = &rc;
  std::sort(rectPtrs.begin(), rectPtrs.end(), by_area);

  gfx::Region rgn(m_bounds);
  i = 0;
  for (auto rcPtr : rectPtrs) {
    if (token.canceled())
      return false;
    token.set_progress(float(i) / int(rectPtrs.size()));

    gfx::Rect& rc = *rcPtr;

    // The rectangles are treated as its original size +
    // conditional extra border of <shapePadding> during placement.
    for (int v=0; v<=m_bounds.h-rc.h; ++v) {
      for (int u=0; u<=m_bounds.w-rc.w; ++u) {
        if (token.canceled())
          return false;

        // It's necessary to consider the <shapePadding> as an
        // integral part of the image size; otherwise, the region
        // subtraction process may be incorrect, resulting in
        // overlapping of shape padding between adjacent sprites.
        // This fix resolves the special cases of exporting with
        // sheet type 'Packed' + 'Trim Cels' true +
        // 'Shape padding' > 0 + series of particular image sizes.
        int wShapePadding =
          ((u + rc.w) == m_bounds.w ? 0 : m_shapePadding);
        int hShapePadding =
          ((v + rc.h) == m_bounds.h ? 0 : m_shapePadding);
        gfx::Rect possible(m_bounds.x + u,
                           m_bounds.y + v,
                           rc.w + wShapePadding,
                           rc.h + hShapePadding);

        Region::Overlap overlap = rgn.contains(possible);
        if (overlap == Region::In) {
          rc = Rect(m_bounds.x + u, m_bounds.y + v, rc.w, rc.h);
          rgn.createSubtraction(rgn, gfx::Region(Rect(possible)));
          goto next_rc;
        }
      }
    }
    return false; // There is not enough room for "rc"
  next_rc:;
    ++i;
  }

  return true;
}

} // namespace gfx

// LAF Gfx Library
// Copyright (C) 2019-2022 Igara Studio S.A.
// Copyright (C) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "gfx/border.h"
#include "gfx/rect.h"
#include "gfx/rect_io.h"
#include "gfx/size.h"

using namespace std;
using namespace gfx;

TEST(Rect, Ctor)
{
  EXPECT_EQ(Rect(0, 0, 0, 0), Rect());
  EXPECT_EQ(10, Rect(10, 20, 30, 40).x);
  EXPECT_EQ(20, Rect(10, 20, 30, 40).y);
  EXPECT_EQ(30, Rect(10, 20, 30, 40).w);
  EXPECT_EQ(40, Rect(10, 20, 30, 40).h);
}

TEST(Rect, Inflate)
{
  EXPECT_EQ(Rect(10, 20, 31, 42), Rect(10, 20, 30, 40).inflate(1, 2));
  EXPECT_EQ(Rect(10, 20, 31, 42), Rect(10, 20, 30, 40).inflate(Size(1, 2)));
}

TEST(Rect, Enlarge)
{
  EXPECT_EQ(Rect(9, 19, 32, 42), Rect(10, 20, 30, 40).enlarge(1));
  EXPECT_EQ(Rect(9, 18, 34, 46), Rect(10, 20, 30, 40).enlarge(Border(1, 2, 3, 4)));
  EXPECT_EQ(Rect(9, 18, 34, 46), Rect(10, 20, 30, 40) + Border(1, 2, 3, 4));
}

TEST(Rect, Shrink)
{
  EXPECT_EQ(Rect(11, 21, 28, 38), Rect(10, 20, 30, 40).shrink(1));
  EXPECT_EQ(Rect(11, 22, 26, 34), Rect(10, 20, 30, 40).shrink(Border(1, 2, 3, 4)));
  EXPECT_EQ(Rect(11, 22, 26, 34), Rect(10, 20, 30, 40) - Border(1, 2, 3, 4));
}

TEST(Rect, FitIn)
{
  EXPECT_EQ(Rect(10, 10, 8, 8), Rect(0, 0, 4, 4).fitIn(Rect(10, 10, 8, 8)));
  EXPECT_EQ(Rect(10, 10, 4, 4), Rect(0, 0, 8, 8).fitIn(Rect(10, 10, 4, 4)));

  EXPECT_EQ(Rect(10, 10, 8, 4), Rect(0, 0, 16, 8).fitIn(Rect(10, 10, 8, 4)));
  EXPECT_EQ(Rect(10, 12, 8, 4), Rect(0, 0, 16, 8).fitIn(Rect(10, 10, 8, 8)));
  EXPECT_EQ(Rect(13, 10, 2, 4), Rect(0, 0, 8, 16).fitIn(Rect(10, 10, 8, 4)));
}

TEST(Rect, Floor)
{
  EXPECT_EQ(gfx::Rect(0, 0, 1, 2), gfx::Rect(gfx::RectF(-0.25, -0.75, 1, 2)));
  EXPECT_EQ(gfx::Rect(-1, -1, 1, 2), gfx::RectF(-0.25, -0.75, 1, 2).floor());
}


TEST(Rect, SliceV)
{
  const int x = 3, y = 4;
  gfx::Rect l, r;
  auto rect = gfx::Rect(x, y, 5, 7);
  rect.sliceV(x, l, r);
  EXPECT_EQ(gfx::Rect(x,y,0,7), l);
  EXPECT_EQ(gfx::Rect(x,y,5,7), r);

  rect.sliceV(x-1, l, r);
  EXPECT_EQ(gfx::Rect(0,0,0,0), l);
  EXPECT_EQ(gfx::Rect(x,y,5,7), r);

  rect.sliceV(x+1, l, r);
  EXPECT_EQ(gfx::Rect(x,y,1,7), l);
  EXPECT_EQ(gfx::Rect(x+1,y,4,7), r);

  rect.sliceV(x+4, l, r);
  EXPECT_EQ(gfx::Rect(x,y,4,7), l);
  EXPECT_EQ(gfx::Rect(x+4,y,1,7), r);

  rect.sliceV(x+5, l, r);
  EXPECT_EQ(gfx::Rect(x,y,5,7), l);
  EXPECT_EQ(gfx::Rect(x+5,y,0,7), r);

  rect.sliceV(x+6, l, r);
  EXPECT_EQ(gfx::Rect(x,y,5,7), l);
  EXPECT_EQ(gfx::Rect(0,0,0,0), r);
}

TEST(Rect, SliceH)
{
  const int x = 3, y = 4;
  gfx::Rect t, b;
  auto rect = gfx::Rect(x, y, 5, 7);
  rect.sliceH(y, t, b);
  EXPECT_EQ(gfx::Rect(x,y,5,0), t);
  EXPECT_EQ(gfx::Rect(x,y,5,7), b);

  rect.sliceH(y-1, t, b);
  EXPECT_EQ(gfx::Rect(0,0,0,0), t);
  EXPECT_EQ(gfx::Rect(x,y,5,7), b);

  rect.sliceH(y+1, t, b);
  EXPECT_EQ(gfx::Rect(x,y,5,1), t);
  EXPECT_EQ(gfx::Rect(x,y+1,5,6), b);

  rect.sliceH(y+6, t, b);
  EXPECT_EQ(gfx::Rect(x,y,5,6), t);
  EXPECT_EQ(gfx::Rect(x,y+6,5,1), b);

  rect.sliceH(y+7, t, b);
  EXPECT_EQ(gfx::Rect(x,y,5,7), t);
  EXPECT_EQ(gfx::Rect(x,y+7,5,0), b);

  rect.sliceH(y+8, t, b);
  EXPECT_EQ(gfx::Rect(x,y,5,7), t);
  EXPECT_EQ(gfx::Rect(0,0,0,0), b);
}

TEST(Rect, NineSlice)
{
  const int x = 3, y = 4;
  auto rect = gfx::Rect(x, y, 6, 6);
  gfx::Rect slices[9];

  // Slice using an inner rect.
  rect.nineSlice(gfx::Rect(3, 3, 2, 2), slices);
  EXPECT_EQ(gfx::Rect(x,y,6,6), rect);
  EXPECT_EQ(gfx::Rect(x  ,y  ,3,3), slices[0]);
  EXPECT_EQ(gfx::Rect(x+3,y  ,2,3), slices[1]);
  EXPECT_EQ(gfx::Rect(x+5,y  ,1,3), slices[2]);
  EXPECT_EQ(gfx::Rect(x  ,y+3,3,2), slices[3]);
  EXPECT_EQ(gfx::Rect(x+3,y+3,2,2), slices[4]);
  EXPECT_EQ(gfx::Rect(x+5,y+3,1,2), slices[5]);
  EXPECT_EQ(gfx::Rect(x  ,y+5,3,1), slices[6]);
  EXPECT_EQ(gfx::Rect(x+3,y+5,2,1), slices[7]);
  EXPECT_EQ(gfx::Rect(x+5,y+5,1,1), slices[8]);

  // Slice using a center rect with the same size as the rect being sliced.
  rect.nineSlice(gfx::Rect(0, 0, 6, 6), slices);
  EXPECT_EQ(gfx::Rect(x,y,6,6), rect);
  EXPECT_EQ(gfx::Rect(x  ,y  ,0,0), slices[0]);
  EXPECT_EQ(gfx::Rect(x  ,y  ,6,0), slices[1]);
  EXPECT_EQ(gfx::Rect(x+6,y  ,0,0), slices[2]);
  EXPECT_EQ(gfx::Rect(x  ,y  ,0,6), slices[3]);
  EXPECT_EQ(gfx::Rect(x  ,y  ,6,6), slices[4]);
  EXPECT_EQ(gfx::Rect(x+6,y  ,0,6), slices[5]);
  EXPECT_EQ(gfx::Rect(x  ,y+6,0,0), slices[6]);
  EXPECT_EQ(gfx::Rect(x  ,y+6,6,0), slices[7]);
  EXPECT_EQ(gfx::Rect(x+6,y+6,0,0), slices[8]);

  // Slice using an outer rect.
  rect.nineSlice(gfx::Rect(-1, -1, 8, 8), slices);
  EXPECT_EQ(gfx::Rect(x,y,6,6), rect);
  EXPECT_EQ(gfx::Rect(0,0,0,0), slices[0]);
  EXPECT_EQ(gfx::Rect(0,0,0,0), slices[1]);
  EXPECT_EQ(gfx::Rect(0,0,0,0), slices[2]);
  EXPECT_EQ(gfx::Rect(0,0,0,0), slices[3]);
  EXPECT_EQ(gfx::Rect(x,y,6,6), slices[4]);
  EXPECT_EQ(gfx::Rect(0,0,0,0), slices[5]);
  EXPECT_EQ(gfx::Rect(0,0,0,0), slices[6]);
  EXPECT_EQ(gfx::Rect(0,0,0,0), slices[7]);
  EXPECT_EQ(gfx::Rect(0,0,0,0), slices[8]);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

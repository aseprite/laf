// LAF Base Library
// Copyright (c) 2019 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <gtest/gtest.h>

#include "base/gcd.h"

TEST(GreatCommonDivisor, RandomTests)
{
  EXPECT_EQ(base::gcd(98, 56), 14);
  EXPECT_EQ(base::gcd(56, 98), 14);
  EXPECT_EQ(base::gcd(9, 5), 1);
  EXPECT_EQ(base::gcd(540, 24), 12);
  EXPECT_EQ(base::gcd(24, 540), 12);
  EXPECT_EQ(base::gcd(540, 33), 3);
  EXPECT_EQ(base::gcd(33, 540), 3);
  EXPECT_EQ(base::gcd(56, 1), 1);
  EXPECT_EQ(base::gcd(1, 56), 1);
  EXPECT_EQ(base::gcd(56, 0), 1);
  EXPECT_EQ(base::gcd(0, 56), 1);
  EXPECT_EQ(base::gcd(-2, 6), 2);
  EXPECT_EQ(base::gcd(6, -2), 2);
  EXPECT_EQ(base::gcd(37, 37), 37);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

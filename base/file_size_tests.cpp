// LAF Base Library
// Copyright (c) 2026-present Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <gtest/gtest.h>

#include "base/file_content.h"
#include "base/file_handle.h"
#include "base/file_size.h"

using namespace base;

TEST(FileSize, WithFileHandles)
{
  const char* fn = "__temp_file_size.txt";
  write_file_content(fn, (const uint8_t*)"Hello World", 11);

  EXPECT_EQ(11, file_size(fn));

  FileHandle fh(open_file(fn, "r"));
  FILE* f = fh.get();
  EXPECT_EQ(0, base_ftell(f));

  base_fseek(f, 4, SEEK_SET);
  EXPECT_EQ(4, base_ftell(f));

  base_fseek(f, 0, SEEK_END);
  EXPECT_EQ(11, base_ftell(f));

  base_fseek(f, -1, SEEK_CUR);
  EXPECT_EQ(10, base_ftell(f));

  // Check that base::file_size(FILE*) restores the previous position
  EXPECT_EQ(11, file_size(f));
  EXPECT_EQ(10, base_ftell(f));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

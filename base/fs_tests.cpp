// LAF Base Library
// Copyright (c) 2024 Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <gtest/gtest.h>

#include "base/file_content.h"
#include "base/fs.h"

#include <cstdio>

#if !LAF_MACOS
  #define COMPARE_WITH_STD_FS 1
#endif

#if COMPARE_WITH_STD_FS
  // We cannot use the <filesystem> on macOS yet because we are
  // targetting macOS 10.9 platform.
  #include <filesystem>
  namespace fs = std::filesystem;
#endif

using namespace base;

#if COMPARE_WITH_STD_FS
// We want to test against std::filesystem for future replacement of
// some of our functions with the standard ones.
TEST(FS, CurrentPath)
{
  // Compare with <filesystem>
  EXPECT_EQ(fs::current_path(), get_current_path());
  EXPECT_EQ(fs::path::preferred_separator, path_separator);
}
#endif

TEST(FS, FixPathSeparators)
{
  const std::string sep(1, path_separator);
  EXPECT_EQ(sep, fix_path_separators("/"));
  EXPECT_EQ(sep, fix_path_separators("///"));
  EXPECT_EQ(sep+"a"+sep, fix_path_separators("//a/"));
  EXPECT_EQ("a"+sep+"b"+sep, fix_path_separators("a///b/"));

#if LAF_WINDOWS
  EXPECT_EQ("\\\\hostname\\a\\b", fix_path_separators("\\\\hostname\\\\a/b"));
  EXPECT_EQ("\\\\hostname\\b", fix_path_separators("\\\\/hostname\\b"));
#endif
}

TEST(FS, MakeDirectory)
{
  EXPECT_FALSE(is_directory("a"));

  make_directory("a");
  EXPECT_TRUE(is_directory("a"));

  remove_directory("a");
  EXPECT_FALSE(is_directory("a"));
}

TEST(FS, MakeAllDirectories)
{
  EXPECT_FALSE(is_directory("a"));
  EXPECT_FALSE(is_directory("a/b"));
  EXPECT_FALSE(is_directory("a/b/c"));

  make_all_directories("a/b/c");

  EXPECT_TRUE(is_directory("a"));
  EXPECT_TRUE(is_directory("a/b"));
  EXPECT_TRUE(is_directory("a/b/c"));

  remove_directory("a/b/c");
  EXPECT_FALSE(is_directory("a/b/c"));

  remove_directory("a/b");
  EXPECT_FALSE(is_directory("a/b"));

  remove_directory("a");
  EXPECT_FALSE(is_directory("a"));
}

TEST(FS, IsPathSeparator)
{
  EXPECT_TRUE (is_path_separator('/'));
  EXPECT_FALSE(is_path_separator('a'));
  EXPECT_FALSE(is_path_separator('+'));
  EXPECT_FALSE(is_path_separator(':'));

#if LAF_WINDOWS
  EXPECT_TRUE (is_path_separator('\\'));
#else
  EXPECT_FALSE(is_path_separator('\\'));
#endif
}

TEST(FS, GetFilePath)
{
  EXPECT_EQ("C:/foo",   get_file_path("C:/foo/pack.tar.gz"));
  EXPECT_EQ(".",        get_file_path("./main.cpp"));
  EXPECT_EQ("",         get_file_path("\\main.cpp"));
  EXPECT_EQ("",         get_file_path("main.cpp"));
  EXPECT_EQ("",         get_file_path("main."));
  EXPECT_EQ("",         get_file_path("main"));
  EXPECT_EQ("C:/foo",   get_file_path("C:/foo/"));
  EXPECT_EQ("",         get_file_path(".cpp"));
  EXPECT_EQ("",         get_file_path(""));

#if LAF_WINDOWS
  EXPECT_EQ("C:\\foo",  get_file_path("C:\\foo\\main.cpp"));
  EXPECT_EQ(".",        get_file_path(".\\main.cpp"));
  EXPECT_EQ("C:",       get_file_path("C:\\"));
  EXPECT_EQ("C:",       get_file_path("C:\\.cpp"));
#else
  EXPECT_EQ("",         get_file_path("C:\\foo\\main.cpp"));
  EXPECT_EQ("",         get_file_path(".\\main.cpp"));
  EXPECT_EQ("",         get_file_path("C:\\"));
  EXPECT_EQ("",         get_file_path("C:\\.cpp"));
#endif
}

TEST(FS, GetFileName)
{
  EXPECT_EQ("pack.tar.gz",      get_file_name("C:/foo/pack.tar.gz"));
  EXPECT_EQ("main.cpp",         get_file_name("./main.cpp"));
  EXPECT_EQ("main.cpp",         get_file_name("main.cpp"));
  EXPECT_EQ("main.",            get_file_name("main."));
  EXPECT_EQ("main",             get_file_name("main"));
  EXPECT_EQ("",                 get_file_name("C:/foo/"));
  EXPECT_EQ(".cpp",             get_file_name(".cpp"));
  EXPECT_EQ("",                 get_file_name(""));

#if LAF_WINDOWS
  EXPECT_EQ("main.cpp",         get_file_name("C:\\foo\\main.cpp"));
  EXPECT_EQ("main.cpp",         get_file_name(".\\main.cpp"));
  EXPECT_EQ("main.cpp",         get_file_name("\\main.cpp"));
  EXPECT_EQ("",                 get_file_name("C:\\"));
  EXPECT_EQ(".cpp",             get_file_name("C:\\.cpp"));
#else
  EXPECT_EQ("C:\\foo\\main.cpp", get_file_name("C:\\foo\\main.cpp"));
  EXPECT_EQ(".\\main.cpp",      get_file_name(".\\main.cpp"));
  EXPECT_EQ("\\main.cpp",       get_file_name("\\main.cpp"));
  EXPECT_EQ("C:\\",             get_file_name("C:\\"));
  EXPECT_EQ("C:\\.cpp",         get_file_name("C:\\.cpp"));
#endif
}

TEST(FS, GetFileExtension)
{
  EXPECT_EQ("gz",       get_file_extension("C:/foo/pack.tar.gz"));
  EXPECT_EQ("cpp",      get_file_extension("./main.cpp"));
  EXPECT_EQ("cpp",      get_file_extension("main.cpp"));
  EXPECT_EQ("",         get_file_extension("main."));
  EXPECT_EQ("",         get_file_extension("main"));
  EXPECT_EQ("",         get_file_extension("C:/foo/"));
  EXPECT_EQ("cpp",      get_file_extension(".cpp"));
  EXPECT_EQ("",         get_file_extension(""));

  // Same results on Windows/macOS/Linux
  EXPECT_EQ("cpp",      get_file_extension("C:\\foo\\main.cpp"));
  EXPECT_EQ("cpp",      get_file_extension(".\\main.cpp"));
  EXPECT_EQ("cpp",      get_file_extension("\\main.cpp"));
  EXPECT_EQ("",         get_file_extension("C:\\"));
  EXPECT_EQ("cpp",      get_file_extension("C:\\.cpp"));
}

TEST(FS, GetFileTitle)
{
  EXPECT_EQ("pack.tar", get_file_title("C:/foo/pack.tar.gz"));
  EXPECT_EQ("main",     get_file_title("./main.cpp"));
  EXPECT_EQ("main",     get_file_title("main.cpp"));
  EXPECT_EQ("main",     get_file_title("main."));
  EXPECT_EQ("main",     get_file_title("main"));
  EXPECT_EQ("",         get_file_title("C:/foo/"));
  EXPECT_EQ("",         get_file_title(".cpp"));
  EXPECT_EQ("",         get_file_title(""));

#if LAF_WINDOWS
  EXPECT_EQ("main",     get_file_title("C:\\foo\\main.cpp"));
  EXPECT_EQ("main",     get_file_title(".\\main.cpp"));
  EXPECT_EQ("main",     get_file_title("\\main.cpp"));
  EXPECT_EQ("",         get_file_title("C:\\"));
  EXPECT_EQ("",         get_file_title("C:\\.cpp"));
#else
  EXPECT_EQ("C:\\foo\\main", get_file_title("C:\\foo\\main.cpp"));
  EXPECT_EQ(".\\main",  get_file_title(".\\main.cpp"));
  EXPECT_EQ("\\main",   get_file_title("\\main.cpp"));
  EXPECT_EQ("C:\\",     get_file_title("C:\\"));
  EXPECT_EQ("C:\\",     get_file_title("C:\\.cpp"));
#endif
}

TEST(FS, GetFileTitleWithPath)
{
  EXPECT_EQ("C:/foo/pack.tar", get_file_title_with_path("C:/foo/pack.tar.gz"));
  EXPECT_EQ("./main",          get_file_title_with_path("./main.cpp"));
  EXPECT_EQ("main",            get_file_title_with_path("main.cpp"));
  EXPECT_EQ("main",            get_file_title_with_path("main."));
  EXPECT_EQ("main",            get_file_title_with_path("main"));
  EXPECT_EQ("C:/foo/",         get_file_title_with_path("C:/foo/"));
  EXPECT_EQ("",                get_file_title_with_path(".cpp"));
  EXPECT_EQ("",                get_file_title_with_path(""));

  // Same results on Windows/macOS/Linux
  EXPECT_EQ("C:\\foo\\main",   get_file_title_with_path("C:\\foo\\main.cpp"));
  EXPECT_EQ(".\\main",         get_file_title_with_path(".\\main.cpp"));
  EXPECT_EQ("\\main",          get_file_title_with_path("\\main.cpp"));
  EXPECT_EQ("C:\\",            get_file_title_with_path("C:\\"));
  EXPECT_EQ("C:\\",            get_file_title_with_path("C:\\.cpp"));
}

TEST(FS, GetRelativePath)
{
  EXPECT_EQ("C:\\foo\\bar\\test.file", get_relative_path("C:\\foo\\bar\\test.file", "D:\\another\\disk"));

#if LAF_WINDOWS
  EXPECT_EQ("bar\\test.file",           get_relative_path("C:\\foo\\bar\\test.file", "C:\\foo"));
  EXPECT_EQ("C:\\foo\\bar\\test.file",  get_relative_path("C:\\foo\\bar\\test.file", "D:\\another\\disk"));
  EXPECT_EQ("..\\bar\\test.file",       get_relative_path("C:\\foo\\bar\\test.file", "C:\\foo\\another"));
  EXPECT_EQ("..\\..\\bar\\test.file",   get_relative_path("C:\\foo\\bar\\test.file", "C:\\foo\\a\\b"));
#else
  EXPECT_EQ("bar/test.file",            get_relative_path("C:/foo/bar/test.file", "C:/foo"));
  EXPECT_EQ("C:/foo/bar/test.file",     get_relative_path("C:/foo/bar/test.file", "D:/another/disk"));
  EXPECT_EQ("../bar/test.file",         get_relative_path("/foo/bar/test.file", "/foo/another"));
  EXPECT_EQ("../../bar/test.file",      get_relative_path("/foo/bar/test.file", "/foo/a/b"));
#endif
}

TEST(FS, GetAbsolutePath)
{
  const auto cp = get_current_path();

  EXPECT_EQ(join_path(cp, "a"), get_absolute_path("a"));
  EXPECT_EQ(join_path(cp, "a"), get_absolute_path("./a"));
  EXPECT_EQ(cp, get_absolute_path("."));
  EXPECT_EQ(cp, get_absolute_path("./."));
  EXPECT_EQ(cp, get_absolute_path("./a/.."));
  EXPECT_EQ(cp, get_absolute_path(".////."));

#if LAF_WINDOWS
  EXPECT_EQ("C:\\file", get_absolute_path("C:/path/../file"));
#else
  EXPECT_EQ("/file", get_absolute_path("/path/../file"));
#endif
}

TEST(FS, GetCanonicalPath)
{
  const auto cp = get_current_path();

  EXPECT_EQ("", get_canonical_path("./non_existent_file"));
  EXPECT_EQ("", get_canonical_path("non_existent_file"));
  EXPECT_EQ(cp, get_canonical_path("."));

  // Creates a file so get_canonical_path() returns its absolute path
  write_file_content("_test_existing_file.txt", (uint8_t*)"123", 3);
  EXPECT_EQ(join_path(cp, "_test_existing_file.txt"),
            get_canonical_path("_test_existing_file.txt"));
}

TEST(FS, NormalizePath)
{
  const std::string sep(1, path_separator);

  EXPECT_EQ("", normalize_path(""));
  EXPECT_EQ(".", normalize_path("."));
  EXPECT_EQ(".", normalize_path("./."));
  EXPECT_EQ(".", normalize_path(".///./."));
  EXPECT_EQ(".", normalize_path(".///./"));

  EXPECT_EQ("a"+sep, normalize_path("a/."));
  EXPECT_EQ("a"+sep, normalize_path("a/"));
  EXPECT_EQ("a", normalize_path("./a"));
  EXPECT_EQ("a"+sep+"b"+sep+"c", normalize_path("a///b/./c"));

  EXPECT_EQ("..", normalize_path(".."));
  EXPECT_EQ(".."+sep+"..", normalize_path("../.."));
  EXPECT_EQ(".."+sep+"..", normalize_path("../../"));
  EXPECT_EQ(".."+sep+"..", normalize_path(".././.."));
  EXPECT_EQ(".."+sep+"..", normalize_path("./.././../."));

  EXPECT_EQ(".", normalize_path("a/.."));
  EXPECT_EQ("..", normalize_path("../a/.."));
  EXPECT_EQ(".."+sep+"..", normalize_path("../a/../.."));
  EXPECT_EQ("..", normalize_path("a/../.."));
  EXPECT_EQ(sep+"b", normalize_path("/a/../b"));

#if LAF_WINDOWS
  EXPECT_EQ("\\\\hostname\\b", normalize_path("\\\\hostname\\\\a/../b"));
  EXPECT_EQ("\\\\hostname\\b\\a", normalize_path("\\\\/hostname\\b/a"));
#endif
}

#if COMPARE_WITH_STD_FS
TEST(FS, CompareNormalizePathWithStd)
{
  for (const char* sample : { "", ".", "./.", ".///./.", ".///./",
                              "a/.", "a/", "./a", "a///b/./c",
                              "..", "../..",
                              "../../", ".././..", "./.././../.",
                              "a/..", "../a/..", "../a/../..", "a/../..",
                              "/a/../b" }) {
    EXPECT_EQ(fs::path(sample).lexically_normal(),
              normalize_path(sample))
      << "  sample=\"" << sample << "\"";
  }
}
#endif

TEST(FS, JoinPath)
{
  const std::string sep(1, path_separator);

  EXPECT_EQ("",                         join_path("", ""));
  EXPECT_EQ("fn",                       join_path("", "fn"));
  EXPECT_EQ("/fn",                      join_path("/", "fn"));
  EXPECT_EQ("/this"+sep+"fn",           join_path("/this", "fn"));

  EXPECT_EQ("C:\\path"+sep+"fn",        join_path("C:\\path", "fn"));
#if LAF_WINDOWS
  EXPECT_EQ("C:\\path\\fn",             join_path("C:\\path\\", "fn"));
#else
  EXPECT_EQ("C:\\path\\/fn",            join_path("C:\\path\\", "fn"));
#endif
}

TEST(FS, RemovePathSeparator)
{
  EXPECT_EQ("C:/foo",            remove_path_separator("C:/foo/"));
  EXPECT_EQ("C:\\foo\\main.cpp", remove_path_separator("C:\\foo\\main.cpp"));
  EXPECT_EQ("C:\\foo\\main.cpp", remove_path_separator("C:\\foo\\main.cpp/"));

#if LAF_WINDOWS
  EXPECT_EQ("C:\\foo",           remove_path_separator("C:\\foo\\"));
#else
  EXPECT_EQ("C:\\foo\\",         remove_path_separator("C:\\foo\\"));
#endif
}

TEST(FS, HasFileExtension)
{
  EXPECT_TRUE (has_file_extension("hi.png", paths{"png"}));
  EXPECT_FALSE(has_file_extension("hi.png", paths{"pngg"}));
  EXPECT_FALSE(has_file_extension("hi.png", paths{"ppng"}));
  EXPECT_TRUE (has_file_extension("hi.jpeg", paths{"jpg","jpeg"}));
  EXPECT_TRUE (has_file_extension("hi.jpg", paths{"jpg","jpeg"}));
  EXPECT_FALSE(has_file_extension("hi.ase", paths{"jpg","jpeg"}));
  EXPECT_TRUE (has_file_extension("hi.ase", paths{"jpg","jpeg","ase"}));
  EXPECT_TRUE (has_file_extension("hi.ase", paths{"ase","jpg","jpeg"}));

  EXPECT_TRUE (has_file_extension("hi.png", paths{"Png"}));
  EXPECT_TRUE (has_file_extension("hi.pnG", paths{"bmp","PNg"}));
  EXPECT_TRUE (has_file_extension("hi.bmP", paths{"bMP","PNg"}));
}

TEST(FS, ReplaceExtension)
{
  EXPECT_EQ("hi.bmp", replace_extension("hi.png", "bmp"));
  EXPECT_EQ("hi.bmp", replace_extension("hi", "bmp"));
  EXPECT_EQ("hi", replace_extension("hi.bmp", ""));
  EXPECT_EQ("hi", replace_extension("hi", ""));
  EXPECT_EQ(".bmp", replace_extension(".png", "bmp"));
  EXPECT_EQ("/path/hi.bmp", replace_extension("/path/hi.png", "bmp"));
  EXPECT_EQ("/path/hi.bmp", replace_extension("/path/hi", "bmp"));
  EXPECT_EQ("/path/hi", replace_extension("/path/hi.bmp", ""));
  EXPECT_EQ("/path/hi", replace_extension("/path/hi", ""));
  EXPECT_EQ("/.bmp", replace_extension("/.png", "bmp"));
}

TEST(FS, CompareFilenames)
{
  EXPECT_EQ(-1, compare_filenames("a", "b"));
  EXPECT_EQ(-1, compare_filenames("a0", "a1"));
  EXPECT_EQ(-1, compare_filenames("a0", "b1"));
  EXPECT_EQ(-1, compare_filenames("a0.png", "a1.png"));
  EXPECT_EQ(-1, compare_filenames("a1-1.png", "a1-2.png"));
  EXPECT_EQ(-1, compare_filenames("a1-2.png", "a1-10.png"));
  EXPECT_EQ(-1, compare_filenames("a1-64-2.png", "a1-64-10.png"));
  EXPECT_EQ(-1, compare_filenames("a32.txt", "a32l.txt"));
  EXPECT_EQ(-1, compare_filenames("a", "aa"));

  EXPECT_EQ(0, compare_filenames("a", "a"));
  EXPECT_EQ(0, compare_filenames("a", "A"));
  EXPECT_EQ(0, compare_filenames("a1B", "A1b"));
  EXPECT_EQ(0, compare_filenames("a32-16.txt32", "a32-16.txt32"));

  EXPECT_EQ(1, compare_filenames("aa", "a"));
  EXPECT_EQ(1, compare_filenames("b", "a"));
  EXPECT_EQ(1, compare_filenames("a1", "a0"));
  EXPECT_EQ(1, compare_filenames("b1", "a0"));
  EXPECT_EQ(1, compare_filenames("a1.png", "a0.png"));
  EXPECT_EQ(1, compare_filenames("a1-2.png", "a1-1.png"));
  EXPECT_EQ(1, compare_filenames("a1-10.png", "a1-9.png"));
  EXPECT_EQ(1, compare_filenames("a1-64-10.png", "a1-64-9.png"));
}

TEST(FS, CopyFiles)
{
  std::vector<uint8_t> data = { 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd' };
  const std::string dst = "_test_copy_.tmp";

  if (is_file(dst))
    delete_file(dst);

  write_file_content("_test_orig_.tmp", data.data(), data.size());
  copy_file("_test_orig_.tmp", dst, true);

  EXPECT_EQ(data, read_file_content(dst));
}

TEST(FS, ListFiles)
{
  // Prepare files
  make_directory("a");
  EXPECT_TRUE(is_directory("a"));

  make_directory("a/b");
  EXPECT_TRUE(is_directory("a/b"));
  make_directory("a/c");
  EXPECT_TRUE(is_directory("a/b"));

  write_file_content("a/d", (uint8_t*)"123", 3);
  EXPECT_TRUE(is_file("a/d"));

  write_file_content("a/e", (uint8_t*)"321", 3);
  EXPECT_TRUE(is_file("a/e"));

  // Test normal find with asterisk match
  EXPECT_TRUE(list_files("non-existent-folder").empty());
  EXPECT_EQ(list_files("a").size(), 4);

  auto dirs = list_files("a", ItemType::Directories);
  EXPECT_EQ(dirs.size(), 2);
  EXPECT_TRUE(std::find(dirs.begin(), dirs.end(), "b") != dirs.end());
  EXPECT_TRUE(std::find(dirs.begin(), dirs.end(), "c") != dirs.end());

  auto files = list_files("a", ItemType::Files);
  EXPECT_EQ(files.size(), 2);
  EXPECT_TRUE(std::find(files.begin(), files.end(), "d") != files.end());
  EXPECT_TRUE(std::find(files.begin(), files.end(), "e") != files.end());

  // Test pattern matching
  make_directory("a/c-match-me");
  EXPECT_TRUE(is_directory("a/c-match-me"));

  EXPECT_EQ(list_files("a", ItemType::Directories, "c-*-me").size(), 1);
  EXPECT_EQ(list_files("a", ItemType::Directories, "c-match-me").size(), 1);
  EXPECT_EQ(list_files("a", ItemType::Files, "c-*-me").size(), 0);

  write_file_content("a/c-alsomatch-me", (uint8_t*)"321", 3);
  EXPECT_TRUE(is_file("a/c-alsomatch-me"));

  EXPECT_EQ(list_files("a", ItemType::Files, "c-*-me").size(), 1);
  EXPECT_EQ(list_files("a", ItemType::Files, "c-alsomatch-me").size(), 1);
  EXPECT_EQ(list_files("a", ItemType::All, "c-*").size(), 2);
  EXPECT_EQ(list_files("a", ItemType::All, "*-me").size(), 2);
  EXPECT_EQ(list_files("a", ItemType::All, "*match*").size(), 2);
  EXPECT_EQ(list_files("a", ItemType::All).size(), 6);

  // Remove files
  delete_file("a/e");
  delete_file("a/d");
  delete_file("a/c-alsomatch-me");
  remove_directory("a/c-match-me");
  remove_directory("a/c");
  remove_directory("a/b");
  remove_directory("a");
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

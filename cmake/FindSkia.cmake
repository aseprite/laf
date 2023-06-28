# Copyright (C) 2019-2022  Igara Studio S.A.
#
# This file is released under the terms of the MIT license.
# Read LICENSE.txt for more information.

set(SKIA_DIR "" CACHE PATH "Skia source code directory")
if(NOT SKIA_DIR)
  set(SKIA_LIBRARY_DIR "" CACHE PATH "Skia library directory (where libskia.a is located)")
else()
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SKIA_ARCH "x64")
    if(APPLE)
      if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64" OR
         (CMAKE_OSX_ARCHITECTURES STREQUAL "" AND
          CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64"))
        set(SKIA_ARCH "arm64")
      endif()
    endif()
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(SKIA_ARCH "x86")
  endif()
  if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(SKIA_LIBRARY_DIR "${SKIA_DIR}/out/Debug-${SKIA_ARCH}" CACHE PATH "Skia library directory")
  else()
    set(SKIA_LIBRARY_DIR "${SKIA_DIR}/out/Release-${SKIA_ARCH}" CACHE PATH "Skia library directory")
  endif()
endif()

# Skia library
find_library(SKIA_LIBRARY skia PATH "${SKIA_LIBRARY_DIR}")
if(WIN32)
  find_library(SKIA_OPENGL_LIBRARY opengl32)
elseif(APPLE)
  find_library(SKIA_OPENGL_LIBRARY OpenGL NAMES GL)
else()
  find_library(SKIA_OPENGL_LIBRARY opengl NAMES GL)
endif()

# SkShaper module + freetype + harfbuzz + zlib
find_library(SKSHAPER_LIBRARY skshaper PATH "${SKIA_LIBRARY_DIR}")

# Check that if Skia is compiled with libc++, we use -stdlib=libc++
if(UNIX AND NOT APPLE AND EXISTS "${SKIA_LIBRARY_DIR}/args.gn")
  file(READ "${SKIA_LIBRARY_DIR}/args.gn" SKIA_ARGS_GN)
  string(FIND "${SKIA_ARGS_GN}" "-stdlib=libc++" matchres)
  if(${matchres} GREATER_EQUAL 0)
    set(not_using_libcxx_error "")
    set(not_using_clang_error "")

    if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang" OR
       NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      set(not_using_clang_error " You must use Clang compiler: \n\
 \n\
   rm CMakeCache.txt \n\
   export CC=clang \n\
   export CXX=clang++ \n\
   cmake ...\n")
    endif()

    string(FIND "${CMAKE_CXX_FLAGS}" "-stdlib=libc++" matchres2)
    string(FIND "${CMAKE_EXE_LINKER_FLAGS}" "-stdlib=libc++" matchres3)
    if(${matchres2} EQUAL -1 OR ${matchres3} EQUAL -1)
      if(not_using_clang_error)
        set(not_using_libcxx_error " \n")
      endif()
      set(not_using_libcxx_error
        "${not_using_libcxx_error} Skia was compiled with -stdlib=libc++ so you must use: \n\
 \n\
   -DCMAKE_CXX_FLAGS=-stdlib=libc++ \n\
   -DCMAKE_EXE_LINKER_FLAGS=-stdlib=libc++ \n")
    endif()

    if(not_using_libcxx_error OR not_using_clang_error)
      message(FATAL_ERROR
        "----------------------------------------------------------------------"
        "                        INCOMPATIBILITY ERROR "
        "----------------------------------------------------------------------\n"
        "${not_using_clang_error}"
        "${not_using_libcxx_error}"
        " \n"
        " When calling cmake (or modify these variables in your CMakeCache.txt)\n"
        "----------------------------------------------------------------------")
    endif()

  endif ()
endif()

# We require zlib because freetype expects to be linked with zlib
if(NOT ZLIB_LIBRARIES)
  if(UNIX)
    set(ZLIB_LIB_FILE "${CMAKE_CURRENT_BINARY_DIR}/third_party/zlib/lib/${CMAKE_STATIC_LIBRARY_PREFIX}z${CMAKE_STATIC_LIBRARY_SUFFIX}")
  else()
    if(MSVC AND CMAKE_BUILD_TYPE STREQUAL Debug)
      set(ZLIB_MSVC_DEBUG_POSTFIX "d")
    endif()
    set(ZLIB_LIB_FILE "${CMAKE_CURRENT_BINARY_DIR}/third_party/zlib/lib/zlib${ZLIB_MSVC_DEBUG_POSTFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")
  endif()

  include(ExternalProject)
  ExternalProject_Add(zlib-project
    URL https://github.com/aseprite/zlib/archive/refs/tags/v1.2.12.zip
    DOWNLOAD_NAME zlib-1.2.12.zip
    URL_HASH SHA1=35c02072f6e3d673f01df54735d5b6af786e0e84
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/third_party/zlib"
    INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/third_party/zlib"
    BUILD_BYPRODUCTS "${ZLIB_LIB_FILE}"
    CMAKE_CACHE_ARGS
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
      -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
      -DCMAKE_INSTALL_LIBDIR:PATH=<INSTALL_DIR>
      -DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}
      -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
      -DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT})

  ExternalProject_Get_Property(zlib-project install_dir)
  set(ZLIB_INCLUDE_DIRS ${install_dir})

  # Create the directory so changing INTERFACE_INCLUDE_DIRECTORIES doesn't fail
  file(MAKE_DIRECTORY ${ZLIB_INCLUDE_DIRS})

  add_library(zlib STATIC IMPORTED)
  set_target_properties(zlib PROPERTIES
    IMPORTED_LOCATION "${ZLIB_LIB_FILE}"
    INTERFACE_INCLUDE_DIRECTORIES ${ZLIB_INCLUDE_DIRS})
  add_dependencies(zlib zlib-project)

  set(ZLIB_LIBRARY zlib)
  set(ZLIB_LIBRARIES ${ZLIB_LIBRARY})
endif()

if(USE_SHARED_FREETYPE)
find_library(FREETYPE_LIBRARY freetype2)
else()
set(FREETYPE_FOUND ON)
find_library(FREETYPE_LIBRARY freetype2 PATH "${SKIA_LIBRARY_DIR}" NO_DEFAULT_PATH)
set(FREETYPE_LIBRARIES ${FREETYPE_LIBRARY})
set(FREETYPE_INCLUDE_DIRS "${SKIA_DIR}/third_party/externals/freetype/include")
endif()

if(USE_SHARED_HARFBUZZ)
find_library(HARFBUZZ_LIBRARY harfbuzz)
else()
find_library(HARFBUZZ_LIBRARY harfbuzz PATH "${SKIA_LIBRARY_DIR}" NO_DEFAULT_PATH)
set(HARFBUZZ_LIBRARIES ${HARFBUZZ_LIBRARY})
set(HARFBUZZ_INCLUDE_DIRS "${SKIA_DIR}/third_party/externals/harfbuzz/src")
endif()

set(SKIA_LIBRARIES
  ${SKIA_LIBRARY}
  ${SKIA_OPENGL_LIBRARY}
  CACHE INTERNAL "Skia libraries")

add_library(skia INTERFACE)
target_include_directories(skia INTERFACE
  ${SKIA_DIR}
  ${FREETYPE_INCLUDE_DIRS}
  ${HARFBUZZ_INCLUDE_DIRS})
target_link_libraries(skia INTERFACE ${SKIA_LIBRARIES})
target_compile_definitions(skia INTERFACE
  SK_INTERNAL
  SK_GAMMA_SRGB
  SK_GAMMA_APPLY_TO_A8
  SK_SCALAR_TO_FLOAT_EXCLUDED
  SK_ALLOW_STATIC_GLOBAL_INITIALIZERS=1
  SK_SUPPORT_GPU=1
  SK_ENABLE_SKSL=1
  SK_GL=1)

# Freetype is used by skia, and it needs zlib
target_link_libraries(skia INTERFACE ${ZLIB_LIBRARIES})

if(WIN32)
  target_compile_definitions(skia INTERFACE
    SK_BUILD_FOR_WIN
    _CRT_SECURE_NO_WARNINGS)
elseif(APPLE)
  target_compile_definitions(skia INTERFACE
    SK_BUILD_FOR_MAC)
else()
  target_compile_definitions(skia INTERFACE
    SK_BUILD_FOR_UNIX)
endif()

if(APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  target_link_libraries(skia INTERFACE
    ${COCOA_LIBRARY})
endif()

if(UNIX AND NOT APPLE)
  # Change the kN32_SkColorType ordering to BGRA to work in X windows.
  target_compile_definitions(skia INTERFACE
    SK_R32_SHIFT=16)

  # Needed for SkFontMgr on Linux
  find_library(FONTCONFIG_LIBRARY fontconfig)
  target_link_libraries(skia INTERFACE
    ${FONTCONFIG_LIBRARY})
endif()

add_library(skshaper INTERFACE)
target_link_libraries(skshaper INTERFACE ${SKSHAPER_LIBRARY})

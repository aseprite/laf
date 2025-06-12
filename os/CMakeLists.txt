# LAF OS
# Copyright (C) 2018-2024  Igara Studio S.A.
# Copyright (C) 2012-2018  David Capello

######################################################################
# Common source code

set(LAF_OS_SOURCES
  common/event_queue.cpp
  common/main.cpp
  common/system.cpp
  dnd.cpp
  error.cpp
  event.cpp
  none/system.cpp
  window.cpp)
if(WIN32)
  list(APPEND LAF_OS_SOURCES
    win/color_space.cpp
    win/dnd.cpp
    win/event_queue.cpp
    win/keys.cpp
    win/system.cpp
    win/winapi.cpp
    win/window.cpp
    win/window_dde.cpp
    win/wintab.cpp)
elseif(APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(CARBON_LIBRARY Carbon)
  if(COCOA_LIBRARY)
    list(APPEND LAF_OS_SOURCES
      osx/app.mm
      osx/app_delegate.mm
      osx/dnd.mm
      osx/color_space.mm
      osx/event_queue.mm
      osx/keys.mm
      osx/logger.mm
      osx/menus.mm
      osx/system.mm
      osx/view.mm
      osx/window.mm
      gl/gl_context_nsgl.mm)
  endif()
else()
  list(APPEND LAF_OS_SOURCES
    x11/dnd.cpp
    x11/event_queue.cpp
    x11/keys.cpp
    x11/system.cpp
    x11/window.cpp
    x11/x11.cpp
    x11/xinput.cpp)
endif()

######################################################################
# Skia backend

if(LAF_BACKEND STREQUAL "skia")
  if(APPLE)
    add_definitions(-Wno-c++14-extensions)
  endif()
  list(APPEND LAF_OS_SOURCES
    skia/skia_color_space.cpp
    skia/skia_gl.cpp
    skia/skia_surface.cpp
    skia/skia_system.cpp
    skia/skia_window.cpp)
  if(WIN32)
    list(APPEND LAF_OS_SOURCES
      skia/skia_window_win.cpp)
  elseif(APPLE)
    list(APPEND LAF_OS_SOURCES
      skia/skia_window_osx.mm)
  else()
    list(APPEND LAF_OS_SOURCES
      skia/skia_window_x11.cpp)
  endif()
endif()

######################################################################

add_library(laf-os ${LAF_OS_SOURCES})
target_link_libraries(laf-os laf-gfx laf-base)
if(LAF_WITH_CLIP)
  target_link_libraries(laf-os clip)
  target_compile_definitions(laf-os PUBLIC
    LAF_WITH_CLIP)
endif()

set(LAF_OS_PLATFORM_LIBS)
if(WIN32)
  list(APPEND LAF_OS_PLATFORM_LIBS
    kernel32 user32 gdi32 comdlg32 ole32 winmm
    shlwapi psapi wininet comctl32 dbghelp dwmapi
    msimg32)

  # Support IME to input CJK characters
  IF (LAF_WITH_IME)
    target_compile_definitions(laf-os PUBLIC
      LAF_WITH_IME)
  endif()

  # Don't define min/max() macros when including <windows.h>
  target_compile_options(laf-os PRIVATE -DNOMINMAX)

  # To include wacom header files
  target_include_directories(laf-os PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../third_party)
elseif(APPLE)
  if(COCOA_LIBRARY)
    list(APPEND LAF_OS_PLATFORM_LIBS
      ${COCOA_LIBRARY}
      ${CARBON_LIBRARY})
  endif()

  target_compile_options(laf-os PRIVATE
    -Wno-ignored-attributes
    -Wno-unused-result
    -Wno-deprecated-declarations
    -Werror=inconsistent-missing-override
    # Use Automatic Reference Counting
    -fobjc-arc)
else()
  find_package(Threads REQUIRED)
  list(APPEND LAF_OS_PLATFORM_LIBS ${CMAKE_THREAD_LIBS_INIT})

  find_package(X11 REQUIRED)
  target_include_directories(laf-os PRIVATE ${X11_INCLUDE_DIR})
  list(APPEND LAF_OS_PLATFORM_LIBS ${X11_LIBRARIES})
  if(NOT X11_Xcursor_FOUND)
    message(FATAL_ERROR "Xcursor library not found")
  endif()
  if(NOT X11_Xinput_FOUND)
    message(FATAL_ERROR "Xinput library not found")
  endif()
  list(APPEND LAF_OS_PLATFORM_LIBS ${X11_Xcursor_LIB})

  check_library_exists(X11 XOpenIM "${X11_LIB_SEARCH_PATH}" XIM_FOUND)

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(laf-os PRIVATE -Werror=suggest-override)
  endif()
endif()

target_link_libraries(laf-os ${LAF_OS_PLATFORM_LIBS})

if(LAF_BACKEND STREQUAL "skia")
  target_link_libraries(laf-os skia)
endif()

set(_LAF_BACKEND_INITIAL_LINK_FLAGS "")
if(MSVC)
  # Linking with "wsetargv.obj" to add support to expand filename
  # wildcards in argc/argv.
  if(LAF_BACKEND STREQUAL "skia")
    set(_LAF_BACKEND_INITIAL_LINK_FLAGS "-LINK wsetargv.obj -ENTRY:\"wWinMainCRTStartup\"")
  else()
    set(_LAF_BACKEND_INITIAL_LINK_FLAGS "-LINK wsetargv.obj -ENTRY:\"wmainCRTStartup\"")
  endif()
endif()
set(LAF_BACKEND_LINK_FLAGS "${_LAF_BACKEND_INITIAL_LINK_FLAGS}"
  CACHE STRING "Extra flags to link an application that uses laf-os")
mark_as_advanced(LAF_BACKEND_LINK_FLAGS)

if(LAF_WITH_TESTS)
  laf_find_tests(. laf-os)
endif()

# laf-dlgs
# Copyright (C) 2024-2025  Igara Studio S.A.

add_library(laf-dlgs file_dialog.cpp)
target_link_libraries(laf-dlgs laf-base)

if(WIN32)
  target_sources(laf-dlgs PRIVATE win/file_dialog_win.cpp)
elseif(APPLE)
  target_sources(laf-dlgs PRIVATE file_dialog_osx.mm)
else()
  target_sources(laf-dlgs PRIVATE file_dialog_x11.cpp)
endif()

if(LAF_WITH_DLGS_PROC)
  add_executable(laf-dlgs-proc win/dlgs_process.cpp)
  target_link_libraries(laf-dlgs-proc laf-base laf-dlgs)
  set_target_properties(laf-dlgs-proc PROPERTIES
    OUTPUT_NAME ${LAF_DLGS_PROC_NAME})

  target_compile_definitions(laf-dlgs PRIVATE
    -DLAF_DLGS_PROC_NAME="${LAF_DLGS_PROC_NAME}")
  target_sources(laf-dlgs PRIVATE win/file_dialog_win_safe.cpp)
endif()

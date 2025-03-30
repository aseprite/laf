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
  target_link_libraries(laf-dlgs-proc laf-base laf-dlgs
  #   # shlwapi
  #   kernel32 user32 gdi32 comdlg32 ole32 winmm
  #   shlwapi psapi wininet comctl32 dbghelp dwmapi
  #   msimg32
  )
  # target_link_options(laf-dlgs-proc PUBLIC -SUBSYSTEM:CONSOLE -ENTRY:wmain)
  # target_link_options(laf-dlgs-proc PUBLIC -ENTRY:wmain)
  set_target_properties(laf-dlgs-proc PROPERTIES
    OUTPUT_NAME ${LAF_DLGS_PROC_NAME})

  target_compile_definitions(laf-dlgs PRIVATE
    -DLAF_DLGS_PROC_NAME="${LAF_DLGS_PROC_NAME}")
  target_sources(laf-dlgs PRIVATE win/file_dialog_win_safe.cpp)

# if(LAF_WITH_DLGS_PROC)
#   # When we use the laf-dlgs-proc, laf-os depends on it to show the
#   # native file dialog.
#   add_dependencies(laf-os laf-dlgs-proc)
# endif()

endif()

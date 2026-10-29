# wfc-mscomctl: Windows Common Controls (mscomctl.ocx, library MSComctlLib).
#
# The member table is generated from the type-library evidence (scripts/New-UiControlTables.ps1 in WFC).
# The table is built static by default; WFC_MSCOMCTL_SHARED builds it as
# wfcmscomctl.dll so a program can load it at run time instead. The version is the
# one in VERSION and is stamped into the DLL's version resource.
#
# Parent project variables used when present: WFC_OCX_SHARED (the default of
# WFC_MSCOMCTL_SHARED) and the functions wfc_enable_strict_warnings and
# wsp_enable_hardening.

if(NOT DEFINED WFC_OCX_SHARED)
  set(WFC_OCX_SHARED OFF)
endif()
option(
  WFC_MSCOMCTL_SHARED
  "Build the WFC mscomctl library as wfcmscomctl.dll"
  ${WFC_OCX_SHARED}
)

file(
  STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION"
  WFC_MSCOMCTL_VERSION
  LIMIT_COUNT 1
)
set(
  WFC_MSCOMCTL_VERSION
  "${WFC_MSCOMCTL_VERSION}"
  CACHE INTERNAL
  "wfcmscomctl version"
)

set(WFC_MSCOMCTL_SOURCES src/mscomctl_tables.cpp)

if(WFC_MSCOMCTL_SHARED AND WIN32)
  enable_language(RC)
  string(REPLACE "." ";" version_parts "${WFC_MSCOMCTL_VERSION}")
  list(GET version_parts 0 WFC_LIB_VERSION_MAJOR)
  list(GET version_parts 1 WFC_LIB_VERSION_MINOR)
  list(GET version_parts 2 WFC_LIB_VERSION_PATCH)
  set(WFC_LIB_VERSION "${WFC_MSCOMCTL_VERSION}")
  set(WFC_LIB_NAME wfcmscomctl)
  set(WFC_LIB_DESCRIPTION "WFC Windows Common Controls")
  configure_file(
    version.rc.in
    "${CMAKE_CURRENT_BINARY_DIR}/wfcmscomctl.rc"
    @ONLY
  )
  add_library(
    wfc_mscomctl
    SHARED
    ${WFC_MSCOMCTL_SOURCES}
    "${CMAKE_CURRENT_BINARY_DIR}/wfcmscomctl.rc"
  )
  target_compile_definitions(
    wfc_mscomctl
    PUBLIC WFC_MSCOMCTL_SHARED
    PRIVATE WFC_MSCOMCTL_BUILD_DLL
  )
  set_target_properties(
    wfc_mscomctl
    PROPERTIES
      OUTPUT_NAME wfcmscomctl
      VERSION "${WFC_MSCOMCTL_VERSION}"
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
else()
  add_library(wfc_mscomctl STATIC ${WFC_MSCOMCTL_SOURCES})
  set_target_properties(wfc_mscomctl PROPERTIES OUTPUT_NAME wfcmscomctl_static)
endif()
target_include_directories(
  wfc_mscomctl
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features(wfc_mscomctl PRIVATE cxx_std_20)
if(COMMAND wfc_enable_strict_warnings)
  wfc_enable_strict_warnings(wfc_mscomctl)
endif()
if(COMMAND wsp_enable_hardening)
  wsp_enable_hardening(wfc_mscomctl)
endif()

# wfc-comdlg32: Common Dialog (comdlg32.ocx, library MSComDlg).
#
# The member table is written by hand from the documented members.
# The table is built static by default; WFC_COMDLG32_SHARED builds it as
# wfccomdlg32.dll so a program can load it at run time instead. The version is the
# one in VERSION and is stamped into the DLL's version resource.
#
# Parent project variables used when present: WFC_OCX_SHARED (the default of
# WFC_COMDLG32_SHARED) and the functions wfc_enable_strict_warnings and
# wsp_enable_hardening.

if(NOT DEFINED WFC_OCX_SHARED)
  set(WFC_OCX_SHARED OFF)
endif()
option(
  WFC_COMDLG32_SHARED
  "Build the WFC comdlg32 library as wfccomdlg32.dll"
  ${WFC_OCX_SHARED}
)

file(
  STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION"
  WFC_COMDLG32_VERSION
  LIMIT_COUNT 1
)
set(
  WFC_COMDLG32_VERSION
  "${WFC_COMDLG32_VERSION}"
  CACHE INTERNAL
  "wfccomdlg32 version"
)

set(WFC_COMDLG32_SOURCES src/comdlg32_tables.cpp)

if(WFC_COMDLG32_SHARED AND WIN32)
  enable_language(RC)
  string(REPLACE "." ";" version_parts "${WFC_COMDLG32_VERSION}")
  list(GET version_parts 0 WFC_LIB_VERSION_MAJOR)
  list(GET version_parts 1 WFC_LIB_VERSION_MINOR)
  list(GET version_parts 2 WFC_LIB_VERSION_PATCH)
  set(WFC_LIB_VERSION "${WFC_COMDLG32_VERSION}")
  set(WFC_LIB_NAME wfccomdlg32)
  set(WFC_LIB_DESCRIPTION "WFC Common Dialog")
  configure_file(
    version.rc.in
    "${CMAKE_CURRENT_BINARY_DIR}/wfccomdlg32.rc"
    @ONLY
  )
  add_library(
    wfc_comdlg32
    SHARED
    ${WFC_COMDLG32_SOURCES}
    "${CMAKE_CURRENT_BINARY_DIR}/wfccomdlg32.rc"
  )
  target_compile_definitions(
    wfc_comdlg32
    PUBLIC WFC_COMDLG32_SHARED
    PRIVATE WFC_COMDLG32_BUILD_DLL
  )
  set_target_properties(
    wfc_comdlg32
    PROPERTIES
      OUTPUT_NAME wfccomdlg32
      VERSION "${WFC_COMDLG32_VERSION}"
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
else()
  add_library(wfc_comdlg32 STATIC ${WFC_COMDLG32_SOURCES})
  set_target_properties(wfc_comdlg32 PROPERTIES OUTPUT_NAME wfccomdlg32_static)
endif()
target_include_directories(
  wfc_comdlg32
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features(wfc_comdlg32 PRIVATE cxx_std_20)
if(COMMAND wfc_enable_strict_warnings)
  wfc_enable_strict_warnings(wfc_comdlg32)
endif()
if(COMMAND wsp_enable_hardening)
  wsp_enable_hardening(wfc_comdlg32)
endif()

# wfc-ocx: the control libraries of WFC -- the member tables of the VB and
# MSComctlLib controls (generated from the type-library evidence), the Common
# Dialog and Winsock controls, and the Winsock socket layer. comctl.inc and
# ocx.inc (include/wfc/ocx) are the behaviour of those controls, compiled into
# the evaluator that includes them.
#
# The tables are built static by default; WFC_OCX_SHARED builds them as
# wfcocx.dll so a program can load them at run time instead.
#
# Parent project variables used when present: WFC_BUILD_TESTS and the
# functions wfc_enable_strict_warnings and wsp_enable_hardening.

option(WFC_OCX_SHARED "Build the WFC control tables as wfcocx.dll" OFF)

file(
  STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION"
  WFC_OCX_VERSION
  LIMIT_COUNT 1
)
set(WFC_OCX_VERSION "${WFC_OCX_VERSION}" CACHE INTERNAL "wfcocx version")

function(wfc_ocx_harden target)
  target_compile_features(${target} PRIVATE cxx_std_20)
  if(COMMAND wfc_enable_strict_warnings)
    wfc_enable_strict_warnings(${target})
  endif()
  if(COMMAND wsp_enable_hardening)
    wsp_enable_hardening(${target})
  endif()
endfunction()

set(
  wfc_ocx_table_sources
  src/control_tables.cpp
  src/comctl_tables.cpp
  src/ocx_tables.cpp
)

if(WFC_OCX_SHARED AND WIN32)
  enable_language(RC)
  string(REPLACE "." ";" version_parts "${WFC_OCX_VERSION}")
  list(GET version_parts 0 WFC_LIB_VERSION_MAJOR)
  list(GET version_parts 1 WFC_LIB_VERSION_MINOR)
  list(GET version_parts 2 WFC_LIB_VERSION_PATCH)
  set(WFC_LIB_VERSION "${WFC_OCX_VERSION}")
  set(WFC_LIB_NAME wfcocx)
  set(WFC_LIB_DESCRIPTION "WFC control tables")
  configure_file(version.rc.in "${CMAKE_CURRENT_BINARY_DIR}/wfcocx.rc" @ONLY)
  add_library(
    wfc_ocx_tables
    SHARED
    ${wfc_ocx_table_sources}
    "${CMAKE_CURRENT_BINARY_DIR}/wfcocx.rc"
  )
  target_compile_definitions(
    wfc_ocx_tables
    PUBLIC WFC_OCX_SHARED
    PRIVATE WFC_OCX_BUILD_DLL
  )
  set_target_properties(
    wfc_ocx_tables
    PROPERTIES
      OUTPUT_NAME wfcocx
      VERSION "${WFC_OCX_VERSION}"
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
else()
  add_library(wfc_ocx_tables STATIC ${wfc_ocx_table_sources})
  set_target_properties(wfc_ocx_tables PROPERTIES OUTPUT_NAME wfcocx_tables)
endif()
target_include_directories(
  wfc_ocx_tables
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
wfc_ocx_harden(wfc_ocx_tables)

# The lookups over the tables and the Winsock socket layer stay in the program.
add_library(wfc_ocx STATIC src/control_lookup.cpp src/winsock_io.cpp)
target_link_libraries(wfc_ocx PUBLIC wfc_ocx_tables)
if(WIN32)
  target_link_libraries(wfc_ocx PUBLIC ws2_32)
endif()
wfc_ocx_harden(wfc_ocx)

# wfc-mswinsck: Winsock (mswinsck.ocx, library MSWinsockLib).
#
# The member table is written by hand from the documented members;
# src/winsock_io.cpp is the socket layer. The table is built static by
# default; WFC_MSWINSCK_SHARED builds it as wfcmswinsck.dll so a program can
# load it at run time instead. The version is the one in VERSION and is
# stamped into the DLL's version resource.
#
# Parent project variables used when present: WFC_OCX_SHARED (the default of
# WFC_MSWINSCK_SHARED) and the functions wfc_enable_strict_warnings and
# wsp_enable_hardening.

if(NOT DEFINED WFC_OCX_SHARED)
  set(WFC_OCX_SHARED OFF)
endif()
option(
  WFC_MSWINSCK_SHARED
  "Build the WFC mswinsck library as wfcmswinsck.dll"
  ${WFC_OCX_SHARED}
)

file(
  STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION"
  WFC_MSWINSCK_VERSION
  LIMIT_COUNT 1
)
set(
  WFC_MSWINSCK_VERSION
  "${WFC_MSWINSCK_VERSION}"
  CACHE INTERNAL
  "wfcmswinsck version"
)

set(WFC_MSWINSCK_SOURCES src/mswinsck_tables.cpp)

if(WFC_MSWINSCK_SHARED AND WIN32)
  enable_language(RC)
  string(REPLACE "." ";" version_parts "${WFC_MSWINSCK_VERSION}")
  list(GET version_parts 0 WFC_LIB_VERSION_MAJOR)
  list(GET version_parts 1 WFC_LIB_VERSION_MINOR)
  list(GET version_parts 2 WFC_LIB_VERSION_PATCH)
  set(WFC_LIB_VERSION "${WFC_MSWINSCK_VERSION}")
  set(WFC_LIB_NAME wfcmswinsck)
  set(WFC_LIB_DESCRIPTION "WFC Winsock")
  configure_file(
    version.rc.in
    "${CMAKE_CURRENT_BINARY_DIR}/wfcmswinsck.rc"
    @ONLY
  )
  add_library(
    wfc_mswinsck
    SHARED
    ${WFC_MSWINSCK_SOURCES}
    "${CMAKE_CURRENT_BINARY_DIR}/wfcmswinsck.rc"
  )
  target_compile_definitions(
    wfc_mswinsck
    PUBLIC WFC_MSWINSCK_SHARED
    PRIVATE WFC_MSWINSCK_BUILD_DLL
  )
  set_target_properties(
    wfc_mswinsck
    PROPERTIES
      OUTPUT_NAME wfcmswinsck
      VERSION "${WFC_MSWINSCK_VERSION}"
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
else()
  add_library(wfc_mswinsck STATIC ${WFC_MSWINSCK_SOURCES})
  set_target_properties(wfc_mswinsck PROPERTIES OUTPUT_NAME wfcmswinsck_static)
endif()
target_include_directories(
  wfc_mswinsck
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features(wfc_mswinsck PRIVATE cxx_std_20)

# The socket layer is not part of the DLL: it stays in the program.
add_library(wfc_mswinsck_io STATIC src/winsock_io.cpp)
target_include_directories(
  wfc_mswinsck_io
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features(wfc_mswinsck_io PRIVATE cxx_std_20)
if(WIN32)
  target_link_libraries(wfc_mswinsck_io PUBLIC ws2_32)
endif()
if(COMMAND wfc_enable_strict_warnings)
  wfc_enable_strict_warnings(wfc_mswinsck_io)
endif()
if(COMMAND wsp_enable_hardening)
  wsp_enable_hardening(wfc_mswinsck_io)
endif()
target_link_libraries(wfc_mswinsck INTERFACE wfc_mswinsck_io)
if(COMMAND wfc_enable_strict_warnings)
  wfc_enable_strict_warnings(wfc_mswinsck)
endif()
if(COMMAND wsp_enable_hardening)
  wsp_enable_hardening(wfc_mswinsck)
endif()

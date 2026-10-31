# wfc-mscomctl2: Windows Common Controls-2 (mscomct2.ocx, library MSComCtl2).
#
# The member table is generated from the type-library evidence (scripts/New-UiControlTables.ps1 in WFC).
# The table is built static by default; WFC_MSCOMCTL2_SHARED builds it as
# wfcmscomctl2.dll so a program can load it at run time instead. The version is the
# one in VERSION and is stamped into the DLL's version resource.
#
# Parent project variables used when present: WFC_OCX_SHARED (the default of
# WFC_MSCOMCTL2_SHARED) and the functions wfc_enable_strict_warnings and
# wsp_enable_hardening.

# Standalone build: the library outside WFC. WFC itself adds this directory
# with add_subdirectory and supplies the project, CTest and the WSP hardening.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.28)
  project(wfc-mscomctl2 LANGUAGES CXX)
  if(WFC_WINELIB)
    # winegcc writes name.exe (a launcher) and name.exe.so; the platform module
    # resets the suffix when the project is declared, so it is set here.
    set(CMAKE_EXECUTABLE_SUFFIX ".exe")
  endif()
  option(WFC_BUILD_TESTS "Build the tests" ON)
  include(CTest)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
  include(WfcWarnings)
  # WSP-SEC-0015: the pinned WSP baseline supplies the hardening flags (not for
  # the WebAssembly and Winelib targets, whose toolchains do not take them).
  if(
    EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/wsp/tools/cmake/WspBuild.cmake"
    AND NOT EMSCRIPTEN
    AND NOT WFC_WINELIB
  )
    include("${CMAKE_CURRENT_SOURCE_DIR}/wsp/tools/cmake/WspBuild.cmake")
  endif()
endif()

if(NOT DEFINED WFC_OCX_SHARED)
  set(WFC_OCX_SHARED OFF)
endif()
option(
  WFC_MSCOMCTL2_SHARED
  "Build the WFC mscomctl2 library as wfcmscomctl2.dll"
  ${WFC_OCX_SHARED}
)

file(
  STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION"
  WFC_MSCOMCTL2_VERSION
  LIMIT_COUNT 1
)
set(
  WFC_MSCOMCTL2_VERSION
  "${WFC_MSCOMCTL2_VERSION}"
  CACHE INTERNAL
  "wfcmscomctl2 version"
)

set(WFC_MSCOMCTL2_SOURCES src/mscomctl2_tables.cpp)

if(WFC_MSCOMCTL2_SHARED AND WIN32)
  enable_language(RC)
  string(REPLACE "." ";" version_parts "${WFC_MSCOMCTL2_VERSION}")
  list(GET version_parts 0 WFC_LIB_VERSION_MAJOR)
  list(GET version_parts 1 WFC_LIB_VERSION_MINOR)
  list(GET version_parts 2 WFC_LIB_VERSION_PATCH)
  set(WFC_LIB_VERSION "${WFC_MSCOMCTL2_VERSION}")
  file(
    STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/TARGET"
    WFC_LIB_TARGET
    LIMIT_COUNT 1
  )
  configure_file(
    src/registration_info.hpp.in
    "${CMAKE_CURRENT_BINARY_DIR}/registration_info.hpp"
    @ONLY
  )
  set(WFC_LIB_NAME wfcmscomctl2)
  set(WFC_LIB_DESCRIPTION "WFC Windows Common Controls-2")
  configure_file(
    version.rc.in
    "${CMAKE_CURRENT_BINARY_DIR}/wfcmscomctl2.rc"
    @ONLY
  )
  add_library(
    wfc_mscomctl2
    SHARED
    ${WFC_MSCOMCTL2_SOURCES}
    src/registration.cpp
    src/wfcmscomctl2.def
    "${CMAKE_CURRENT_BINARY_DIR}/wfcmscomctl2.rc"
  )
  target_include_directories(wfc_mscomctl2 PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
  target_link_libraries(wfc_mscomctl2 PRIVATE advapi32)
  target_compile_definitions(
    wfc_mscomctl2
    PUBLIC WFC_MSCOMCTL2_SHARED
    PRIVATE WFC_MSCOMCTL2_BUILD_DLL
  )
  set_target_properties(
    wfc_mscomctl2
    PROPERTIES
      OUTPUT_NAME wfcmscomctl2
      VERSION "${WFC_MSCOMCTL2_VERSION}"
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
else()
  add_library(wfc_mscomctl2 STATIC ${WFC_MSCOMCTL2_SOURCES})
  set_target_properties(wfc_mscomctl2 PROPERTIES OUTPUT_NAME wfcmscomctl2_static)
endif()
target_include_directories(
  wfc_mscomctl2
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features(wfc_mscomctl2 PRIVATE cxx_std_20)
if(COMMAND wfc_enable_strict_warnings)
  wfc_enable_strict_warnings(wfc_mscomctl2)
endif()
if(COMMAND wsp_enable_hardening)
  wsp_enable_hardening(wfc_mscomctl2)
endif()

# --- Tests --------------------------------------------------------------------
# The table (any build) and, in the shared build, the DLL itself: a C program
# loads wfcmscomctl2.dll, checks its COM entry points and version resource, and
# registers and unregisters it in the registry of the current user.
if(BUILD_TESTING AND WFC_BUILD_TESTS)
  add_executable(mscomctl2_tables_test tests/mscomctl2_tables_test.cpp)
  target_link_libraries(mscomctl2_tables_test PRIVATE wfc_mscomctl2)
  if(WFC_MSCOMCTL2_SHARED)
    # Next to the DLL, so the program finds it.
    set_target_properties(
      mscomctl2_tables_test
      PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
  endif()
  target_compile_features(mscomctl2_tables_test PRIVATE cxx_std_20)
  if(COMMAND wfc_enable_strict_warnings)
    wfc_enable_strict_warnings(mscomctl2_tables_test)
  endif()
  if(COMMAND wsp_enable_hardening)
    wsp_enable_hardening(mscomctl2_tables_test)
  endif()
  add_test(NAME TC-MP0010-mscomctl2-tables COMMAND mscomctl2_tables_test)
  set_tests_properties(
    TC-MP0010-mscomctl2-tables
    PROPERTIES LABELS "unit;mp-0010;mscomctl2" TIMEOUT 120
  )

  if(WIN32 AND WFC_MSCOMCTL2_SHARED)
    enable_language(C)
    add_executable(mscomctl2_dll_test tests/mscomctl2_dll_test.c)
    target_link_libraries(mscomctl2_dll_test PRIVATE version advapi32)
    add_dependencies(mscomctl2_dll_test wfc_mscomctl2)
    if(MSVC)
      target_compile_options(mscomctl2_dll_test PRIVATE /W4 /WX)
    else()
      target_compile_options(mscomctl2_dll_test PRIVATE -Wall -Wextra -Werror)
    endif()
    if(COMMAND wsp_enable_hardening)
      wsp_enable_hardening(mscomctl2_dll_test)
    endif()
    add_test(
      NAME TC-MP0010-mscomctl2-dll
      COMMAND
        mscomctl2_dll_test "$<TARGET_FILE:wfc_mscomctl2>" "${WFC_MSCOMCTL2_VERSION}"
        "${WFC_LIB_TARGET}"
    )
    set_tests_properties(
      TC-MP0010-mscomctl2-dll
      PROPERTIES
        LABELS "integration;mp-0010;mscomctl2"
        RESOURCE_LOCK registry
        SKIP_RETURN_CODE 77
    )
  endif()
endif()

# --- WebAssembly demo ---------------------------------------------------------
# With Emscripten the member table also builds as a module and a page that lists
# the classes (web/index.html); CI opens the page in Chromium for the README
# screenshot.
if(EMSCRIPTEN)
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/TARGET" WFC_WEB_TARGET LIMIT_COUNT 1)
  add_executable(mscomctl2_web web/wasm_api.cpp)
  target_link_libraries(mscomctl2_web PRIVATE wfc_mscomctl2)
  target_compile_features(mscomctl2_web PRIVATE cxx_std_20)
  target_compile_definitions(
    mscomctl2_web
    PRIVATE
      WFC_WEB_VERSION="${WFC_MSCOMCTL2_VERSION}"
      WFC_WEB_TARGET="${WFC_WEB_TARGET}"
  )
  set_target_properties(mscomctl2_web PROPERTIES OUTPUT_NAME mscomctl2 SUFFIX .js)
  target_link_options(
    mscomctl2_web
    PRIVATE
      -sMODULARIZE=1
      -sEXPORT_NAME=create_mscomctl2
      "-sEXPORTED_FUNCTIONS=['_library_name','_library_version','_library_target','_class_count','_class_name','_member_count','_event_count','_member_name','_event_name']"
      "-sEXPORTED_RUNTIME_METHODS=['ccall','UTF8ToString']"
      -sENVIRONMENT=web,node
  )
  add_custom_command(
    TARGET mscomctl2_web
    POST_BUILD
    COMMAND
      "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/web/index.html"
      "$<TARGET_FILE_DIR:mscomctl2_web>/index.html"
  )
endif()

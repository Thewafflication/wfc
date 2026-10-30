# Registers each library DLL with Windows (regsvr32) in the current user's
# registry, checks what was written, registers the Winsock control under the
# Microsoft class id and checks that the unregistration restores everything.
# Usage: cmake -DREGSVR32=<regsvr32.exe> -DLIBRARIES=<name=version|...>
#   -DDLL_DIR=<folder of the DLLs> -DBITS=<32|64> -P run_register_test.cmake
# A library name is the short name (vbrun, mscomctl, comdlg32, mswinsck); its
# DLL is wfc<name>.dll in DLL_DIR and is skipped when absent.
string(REPLACE "|" ";" LIBRARIES "${LIBRARIES}")
if(NOT EXISTS "${REGSVR32}")
  message(STATUS "no regsvr32 for this architecture")
  return()
endif()

function(register dll microsoft_ids unregister)
  set(flags /s)
  if(unregister)
    list(APPEND flags /u)
  endif()
  execute_process(
    COMMAND
      "${CMAKE_COMMAND}" -E env WFC_REGISTER_SCOPE=user
      WFC_REGISTER_MICROSOFT_IDS=${microsoft_ids} "${REGSVR32}" ${flags}
      "${dll}"
    RESULT_VARIABLE result
  )
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "regsvr32 ${flags} ${dll} failed (${result})")
  endif()
endfunction()

# The output of `reg query`, or the empty string when the key is missing.
function(query key output)
  execute_process(
    COMMAND reg.exe query "${key}" /s
    RESULT_VARIABLE result
    OUTPUT_VARIABLE text
    ERROR_QUIET
  )
  if(result EQUAL 0)
    set(${output} "${text}" PARENT_SCOPE)
  else()
    set(${output} "" PARENT_SCOPE)
  endif()
endfunction()

set(tested 0)
foreach(entry IN LISTS LIBRARIES)
  string(REGEX MATCH "^([a-z0-9]+)=(.*)$" matched "${entry}")
  set(name "${CMAKE_MATCH_1}")
  set(version "${CMAKE_MATCH_2}")
  set(dll "${DLL_DIR}/wfc${name}.dll")
  if(NOT EXISTS "${dll}")
    continue()
  endif()
  math(EXPR tested "${tested} + 1")
  set(key "HKCU\\Software\\WFC\\Libraries\\${name}")

  register("${dll}" 0 FALSE)
  query("${key}" text)
  if(NOT text MATCHES "${version}" OR NOT text MATCHES "wfc${name}.dll")
    register("${dll}" 0 TRUE)
    message(FATAL_ERROR "${name}: registration missing or wrong:\n${text}")
  endif()
  if(name STREQUAL "mscomctl" AND BITS EQUAL 64)
    query("HKCU\\Software\\Classes\\WFC.MSComctlLib.ListView\\CLSID" progid)
    if(NOT progid MATCHES "2C1007A3-9018-519F-A902-87E3E466A897")
      register("${dll}" 0 TRUE)
      message(FATAL_ERROR "mscomctl: the WFC ProgID is missing:\n${progid}")
    endif()
  endif()

  if(name STREQUAL "mswinsck" AND BITS EQUAL 64)
    # The Microsoft class id of Winsock now names this library; the
    # unregistration puts back what was there.
    set(winsock_clsid "{248DD896-BB45-11CF-9ABC-0080C7E7B78D}")
    set(microsoft "HKCU\\Software\\Classes\\CLSID\\${winsock_clsid}")
    query("${microsoft}" before)
    if(NOT before STREQUAL "")
      message(STATUS "the Microsoft Winsock id is already in HKCU: skipped")
    else()
      register("${dll}" 1 FALSE)
      query("${microsoft}\\InprocServer32" served)
      if(NOT served MATCHES "wfcmswinsck.dll")
        register("${dll}" 1 TRUE)
        message(FATAL_ERROR "the Microsoft id is not served:\n${served}")
      endif()
    endif()
  endif()

  register("${dll}" 0 TRUE)
  query("${key}" after)
  if(NOT after STREQUAL "")
    message(FATAL_ERROR "${name}: the unregistration left keys:\n${after}")
  endif()
  if(name STREQUAL "mswinsck" AND BITS EQUAL 64)
    query("${microsoft}" left)
    if(NOT left STREQUAL "")
      message(FATAL_ERROR "the Microsoft Winsock id was left:\n${left}")
    endif()
  endif()
endforeach()
if(tested EQUAL 0)
  message(FATAL_ERROR "no library DLL found in ${DLL_DIR}")
endif()
message(STATUS "registered and unregistered ${tested} libraries")

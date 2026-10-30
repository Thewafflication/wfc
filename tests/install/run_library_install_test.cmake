# Installs one library package on its own into a scratch root for the current
# user (Install-WfcLibrary.ps1), checks the versioned folder, install.json and
# the registration with Windows, then removes it again.
# Usage: cmake -DPOWERSHELL=<powershell.exe>
#   -DINSTALLER=<Install-WfcLibrary.ps1>
#   -DDLL=<path of the DLL> -DLIBRARY=<wfcvbrun|...> -DVERSION=<x.y.z>
#   -DARCH=<x64|x86|arm64> -DWORK=<dir> -P run_library_install_test.cmake
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}/pkg/${ARCH}")
file(COPY "${DLL}" DESTINATION "${WORK}/pkg/${ARCH}")
string(REGEX REPLACE "^wfc" "" short "${LIBRARY}")
set(key "HKCU\\Software\\WFC\\Libraries\\${short}")

function(install_library)
  execute_process(
    COMMAND
      "${POWERSHELL}" -NoProfile -ExecutionPolicy Bypass -File "${INSTALLER}"
      -Source "${WORK}/pkg" -Library ${LIBRARY} -Scope User -Root "${WORK}/root"
      ${ARGN}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "installer failed (${result}): ${out} ${err}")
  endif()
endfunction()

function(registered output)
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

set(folder "${WORK}/root/lib/${LIBRARY}/${VERSION}/${ARCH}")
install_library()
if(NOT EXISTS "${folder}/${LIBRARY}.dll")
  message(FATAL_ERROR "the library is not in ${folder}")
endif()
file(READ "${WORK}/root/install.json" record)
if(NOT record MATCHES "\"${LIBRARY} ${ARCH}\": +\"${VERSION}\"")
  message(FATAL_ERROR "install.json does not list ${LIBRARY}:\n${record}")
endif()
registered(text)
if(NOT text MATCHES "${VERSION}" OR NOT text MATCHES "${LIBRARY}.dll")
  install_library(-Uninstall)
  message(FATAL_ERROR "${LIBRARY} is not registered:\n${text}")
endif()

install_library(-Uninstall)
if(EXISTS "${folder}")
  message(FATAL_ERROR "the uninstall left ${folder}")
endif()
registered(text)
if(NOT text STREQUAL "")
  message(FATAL_ERROR "the uninstall left the registration:\n${text}")
endif()
file(READ "${WORK}/root/install.json" record)
if(record MATCHES "${LIBRARY} ${ARCH}")
  message(FATAL_ERROR "install.json still lists ${LIBRARY}:\n${record}")
endif()
message(STATUS "${LIBRARY} ${VERSION} installed, registered and removed")

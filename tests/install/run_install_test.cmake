# Installs a package built from the build tree into a scratch root (files only:
# the PATH, the Start menu and the registry are left alone), checks the layout
# and the versioned libraries, builds a dynamically linked program with the
# installed wfc, runs it so the launcher finds the runtime in the installation,
# and uninstalls.
# Usage: cmake -DPOWERSHELL=<powershell.exe> -DINSTALLER=<Install-Wfc.ps1>
#   -DBIN=<folder of wfc.exe> -DARCH=<x64|x86|arm64> -DPROJECT=<.vbp>
#   -DWORK=<dir> -P run_install_test.cmake
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}/pkg/${ARCH}" "${WORK}/out")
foreach(
  name
  wfc.exe
  wfcl.exe
  wfcrt.dll
  wfcvbrun.dll
  wfcmscomctl.dll
  wfccomdlg32.dll
  wfcmswinsck.dll
)
  if(EXISTS "${BIN}/${name}")
    file(COPY "${BIN}/${name}" DESTINATION "${WORK}/pkg/${ARCH}")
  endif()
endforeach()

function(run_installer)
  execute_process(
    COMMAND
      "${POWERSHELL}" -NoProfile -ExecutionPolicy Bypass -File "${INSTALLER}"
      ${ARGN}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "installer failed (${result}): ${out} ${err}")
  endif()
endfunction()

set(root "${WORK}/root")
run_installer(
  -Source
  "${WORK}/pkg"
  -Root
  "${root}"
  -Scope
  User
  -FilesOnly
)
foreach(
  file
  bin/wfc.exe
  bin/wfcl.exe
  bin/wfc-prompt.cmd
  install.json
  Install-Wfc.ps1
  bin/${ARCH}/wfc.exe
)
  if(NOT EXISTS "${root}/${file}")
    message(FATAL_ERROR "the install did not write ${file}")
  endif()
endforeach()
file(GLOB runtime_dll "${root}/lib/wfcrt/*/${ARCH}/wfcrt.dll")
file(GLOB vbrun_dll "${root}/lib/wfcvbrun/*/${ARCH}/wfcvbrun.dll")
if(NOT runtime_dll OR NOT vbrun_dll)
  message(FATAL_ERROR "versioned library folders missing below ${root}/lib")
endif()
file(READ "${root}/install.json" record)
if(NOT record MATCHES "\"wfcrt ${ARCH}\"")
  message(FATAL_ERROR "install.json does not list the libraries:\n${record}")
endif()

# The installed wfc finds its libraries in the versioned folders.
set(ENV{WFC_HOME} "${root}")
execute_process(
  COMMAND "${root}/bin/wfc.exe" libs
  OUTPUT_VARIABLE libs
  RESULT_VARIABLE result
)
if(NOT result EQUAL 0 OR NOT libs MATCHES "wfcrt +[0-9.]+ +dynamic")
  message(FATAL_ERROR "wfc libs does not see the installation:\n${libs}")
endif()

# A dynamic build with the installed wfc, no --bundle: the launcher loads the
# runtime from the installation.
execute_process(
  COMMAND
    "${root}/bin/wfc.exe" build "${PROJECT}" -o "${WORK}/out/app.exe" --native
    --link dynamic
  RESULT_VARIABLE result
  ERROR_VARIABLE err
)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "build with the installed wfc failed: ${err}")
endif()
execute_process(
  COMMAND "${WORK}/out/app.exe"
  OUTPUT_VARIABLE printed
  ERROR_VARIABLE err
  RESULT_VARIABLE result
)
if(NOT result EQUAL 0 OR NOT printed MATCHES "printed")
  message(FATAL_ERROR "the program did not run (${result}): ${printed} ${err}")
endif()

run_installer(
  -Root
  "${root}"
  -Scope
  User
  -FilesOnly
  -Uninstall
)
if(EXISTS "${root}/bin" OR EXISTS "${root}/lib")
  message(FATAL_ERROR "the uninstall left bin or lib behind")
endif()

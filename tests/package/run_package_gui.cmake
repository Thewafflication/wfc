# Builds a packaged program with --gui and runs it standalone in real windows
# with the options of its own command line (--exit-after, --screenshots).
# Usage: cmake -DWFC=<wfc> -DPROJECT=<.vbp> -DWORK=<dir> -DFORM=<name>
#   [-DSEED=<WFC_RANDOM_SEED>] -P run_package_gui.cmake
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")
set(app "${WORK}/standalone.exe")
# NATIVE builds the program as machine code (MP-0009), otherwise it is
# packaged for the interpreter. LINK=dynamic makes the small launcher program
# that loads wfcrt.dll, which BUNDLE copies next to it (MP-0010).
set(native_option "--interpret")
if(DEFINED NATIVE)
  set(native_option "--native")
endif()
if(DEFINED LINK)
  list(APPEND native_option --link ${LINK} --bundle)
endif()
execute_process(
  COMMAND "${WFC}" --build "${PROJECT}" -o "${app}" --gui ${native_option}
  RESULT_VARIABLE built
  ERROR_VARIABLE build_errors
)
if(NOT built EQUAL 0)
  message(FATAL_ERROR "--build failed (${built}): ${build_errors}")
endif()
# A program of forms is a Windows program: no console window opens for it.
file(READ "${app}" header LIMIT 4096 HEX)
string(SUBSTRING "${header}" 120 8 lfanew_hex) # e_lfanew at offset 0x3C
string(REGEX MATCHALL ".." lfanew_bytes "${lfanew_hex}")
list(REVERSE lfanew_bytes)
string(JOIN "" lfanew_text ${lfanew_bytes})
math(EXPR lfanew "0x${lfanew_text}")
math(EXPR subsystem_at "(${lfanew} + 24 + 68) * 2")
string(SUBSTRING "${header}" ${subsystem_at} 4 subsystem_hex)
if(NOT subsystem_hex STREQUAL "0200") # IMAGE_SUBSYSTEM_WINDOWS_GUI
  message(
    FATAL_ERROR
    "the --gui program is not a Windows-subsystem program "
    "(subsystem bytes ${subsystem_hex})"
  )
endif()
if(DEFINED SEED)
  set(ENV{WFC_RANDOM_SEED} "${SEED}")
endif()
execute_process(
  COMMAND "${app}" --exit-after 4000 --screenshots "${WORK}/shots"
  RESULT_VARIABLE ran
  ERROR_VARIABLE errors
  WORKING_DIRECTORY "${WORK}"
)
if(NOT ran EQUAL 0 OR NOT "${errors}" STREQUAL "")
  message(FATAL_ERROR "the packaged program failed (${ran}): ${errors}")
endif()
if(NOT EXISTS "${WORK}/shots/${FORM}-shown.png")
  message(FATAL_ERROR "the form ${FORM} was not shown")
endif()

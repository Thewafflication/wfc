# Builds a packaged program with --gui and runs it standalone in real windows
# with the options of its own command line (--exit-after, --screenshots).
# Usage: cmake -DWFC=<wfc> -DPROJECT=<.vbp> -DWORK=<dir> -DFORM=<name>
#   [-DSEED=<WFC_RANDOM_SEED>] -P run_package_gui.cmake
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")
set(app "${WORK}/standalone.exe")
# NATIVE builds the program as machine code (MP-0009).
set(native_option "")
if(DEFINED NATIVE)
  set(native_option "--native")
endif()
execute_process(
  COMMAND "${WFC}" --build "${PROJECT}" -o "${app}" --gui ${native_option}
  RESULT_VARIABLE built
  ERROR_VARIABLE build_errors
)
if(NOT built EQUAL 0)
  message(FATAL_ERROR "--build failed (${built}): ${build_errors}")
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

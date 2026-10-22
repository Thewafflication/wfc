# Runs a real project headless with a scripted input file and checks that it
# ends cleanly and quietly (no run-time error, no diagnostics).
# Usage: cmake -DWFC=<wfc> -DPROJECT=<.vbp> -DSCRIPT=<.ui> -DWORK=<dir>
#   [-DEXPECTED=<file>] -P run_project_script.cmake
# EXPECTED holds the standard output the program should print.
get_filename_component(project_dir "${PROJECT}" DIRECTORY)
get_filename_component(project_file "${PROJECT}" NAME)
file(REMOVE_RECURSE "${WORK}")
file(COPY "${project_dir}/" DESTINATION "${WORK}")
# A fixed seed makes `Randomize` repeatable for programs that play at random.
if(DEFINED SEED)
  set(ENV{WFC_RANDOM_SEED} "${SEED}")
endif()
# A fixed screen size keeps a layout that follows the screen repeatable.
if(DEFINED SCREEN)
  set(ENV{WFC_SCREEN} "${SCREEN}")
endif()
execute_process(
  COMMAND "${WFC}" "${WORK}/${project_file}" --ui-script "${SCRIPT}"
  RESULT_VARIABLE result
  ERROR_VARIABLE errors
  OUTPUT_VARIABLE output
)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "wfc exited with ${result}: ${errors}")
endif()
if(NOT "${errors}" STREQUAL "")
  message(FATAL_ERROR "unexpected diagnostics: ${errors}")
endif()
if(DEFINED EXPECTED)
  file(READ "${EXPECTED}" expected_text)
  string(
    REPLACE
      "
"
    "
"
    expected_text
    "${expected_text}"
  )
  string(
    REPLACE
      "
"
    "
"
    output
    "${output}"
  )
  string(STRIP "${expected_text}" expected_text)
  string(STRIP "${output}" output)
  if(NOT output STREQUAL expected_text)
    message(
      FATAL_ERROR
      "output mismatch
--- expected ---
${expected_text}
--- actual ---
${output}"
    )
  endif()
endif()

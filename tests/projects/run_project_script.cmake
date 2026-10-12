# Runs a real project headless with a scripted input file and checks that it
# ends cleanly and quietly (no run-time error, no diagnostics).
# Usage: cmake -DWFC=<wfc> -DPROJECT=<.vbp> -DSCRIPT=<.ui> -DWORK=<dir>
#   -P run_project_script.cmake
get_filename_component(project_dir "${PROJECT}" DIRECTORY)
get_filename_component(project_file "${PROJECT}" NAME)
file(REMOVE_RECURSE "${WORK}")
file(COPY "${project_dir}/" DESTINATION "${WORK}")
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

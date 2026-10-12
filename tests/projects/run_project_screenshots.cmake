# Runs a real project in the Win32 host for a few seconds and checks that it
# ends cleanly and shows the named forms (one screenshot each).
# Usage: cmake -DWFC=<wfc> -DPROJECT=<.vbp> -DSHOTS=<dir> -DFORMS=<a;b>
#   [-DMILLISECONDS=5000] -P run_project_screenshots.cmake
if(NOT DEFINED MILLISECONDS)
  set(MILLISECONDS 5000)
endif()
file(REMOVE_RECURSE "${SHOTS}")
# Programs write beside themselves (settings, logs): run a copy, so the
# corpus submodule stays clean.
get_filename_component(project_dir "${PROJECT}" DIRECTORY)
get_filename_component(project_file "${PROJECT}" NAME)
set(copy "${SHOTS}-run")
file(REMOVE_RECURSE "${copy}")
file(COPY "${project_dir}/" DESTINATION "${copy}")
execute_process(
  COMMAND
    "${WFC}" "${copy}/${project_file}" --gui --exit-after ${MILLISECONDS}
    --screenshots "${SHOTS}"
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
foreach(form IN LISTS FORMS)
  set(shot "${SHOTS}/${form}-shown.png")
  if(NOT EXISTS "${shot}")
    message(FATAL_ERROR "the form ${form} was not shown (no ${shot})")
  endif()
  file(SIZE "${shot}" bytes)
  if(bytes LESS 1000)
    message(FATAL_ERROR "${shot} is empty")
  endif()
endforeach()

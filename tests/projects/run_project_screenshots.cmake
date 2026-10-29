# Runs a real project in the Win32 host for a few seconds and checks that it
# ends cleanly and shows the named forms (one screenshot each).
# Usage: cmake -DWFC=<wfc> -DPROJECT=<.vbp> -DSHOTS=<dir> -DFORMS=<a;b>
#   [-DMILLISECONDS=5000] [-DSCRIPT=<.ui>] [-DFILES=<name.png;...>]
#   -P run_project_screenshots.cmake
# SCRIPT drives the real windows; FILES are screenshots the script must take.
if(NOT DEFINED MILLISECONDS)
  set(MILLISECONDS 5000)
endif()
file(REMOVE_RECURSE "${SHOTS}")
# A fixed seed makes `Randomize` repeatable for programs that play at random.
if(DEFINED SEED)
  set(ENV{WFC_RANDOM_SEED} "${SEED}")
endif()
# Programs write beside themselves (settings, logs): run a copy, so the
# corpus submodule stays clean.
get_filename_component(project_dir "${PROJECT}" DIRECTORY)
get_filename_component(project_file "${PROJECT}" NAME)
set(copy "${SHOTS}-run")
file(REMOVE_RECURSE "${copy}")
file(COPY "${project_dir}/" DESTINATION "${copy}")
set(script_arguments "")
if(DEFINED SCRIPT)
  set(script_arguments --ui-script "${SCRIPT}")
endif()
# Games run as machine code (MP-0009) unless NATIVE is OFF.
set(native_option "")
if(NOT DEFINED NATIVE OR NATIVE)
  set(native_option --native)
endif()
execute_process(
  COMMAND
    "${WFC}" ${native_option} "${copy}/${project_file}" --gui --exit-after
    ${MILLISECONDS} --screenshots "${SHOTS}" ${script_arguments}
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
set(shots "")
foreach(form IN LISTS FORMS)
  list(APPEND shots "${form}-shown.png")
endforeach()
list(APPEND shots ${FILES})
foreach(name IN LISTS shots)
  set(shot "${SHOTS}/${name}")
  if(NOT EXISTS "${shot}")
    message(FATAL_ERROR "no screenshot ${shot}")
  endif()
  file(SIZE "${shot}" bytes)
  if(bytes LESS 1000)
    message(FATAL_ERROR "${shot} is empty")
  endif()
endforeach()

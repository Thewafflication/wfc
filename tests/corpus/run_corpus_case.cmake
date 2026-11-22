# Runs one corpus program through wfc and compares stdout with the expected
# text.
# The interpreter runs the program (a project is otherwise compiled).
# Usage: cmake -DWFC=<wfc> -DINPUT=<.bas|.vbp> -DEXPECTED=<file>
#   [-DDEBUG_EXPECTED=<file>] [-DKEYS=<file>] -P run_corpus_case.cmake
# DEBUG_EXPECTED holds the Immediate-window output (Debug.Print) and load
# warnings that wfc writes to stderr.
# KEYS is the standard input of the program: the keys a PDS program reads
# with INKEY$ (ADR-0011). A <name>.exitcode file beside the program gives the
# exit status it must end with (default 0).
set(interpret_option --interpret)
if(PACKAGED)
  set(interpret_option "")
endif()
set(input_option "")
if(DEFINED KEYS)
  set(input_option INPUT_FILE "${KEYS}")
endif()
# A program that ends on an error a handler does not take has a
# <name>.exitcode file beside it with the exit status wfc gives (1).
get_filename_component(case_dir "${INPUT}" DIRECTORY)
get_filename_component(case_name "${INPUT}" NAME_WE)
set(expected_result 0)
if(EXISTS "${case_dir}/${case_name}.exitcode")
  file(
    STRINGS "${case_dir}/${case_name}.exitcode"
    expected_result
    LIMIT_COUNT 1
  )
endif()
execute_process(
  COMMAND "${WFC}" ${interpret_option} "${INPUT}" ${input_option}
  OUTPUT_VARIABLE actual
  ERROR_VARIABLE errors
  RESULT_VARIABLE result
  # wfc writes UTF-8 (the box drawing of a text screen, ADR-0012).
  ENCODING UTF8
)
file(READ "${EXPECTED}" expected)
string(REPLACE "\r\n" "\n" actual "${actual}")
string(REPLACE "\r\n" "\n" expected "${expected}")
string(STRIP "${actual}" actual)
string(STRIP "${expected}" expected)
if(NOT result EQUAL expected_result)
  message(FATAL_ERROR "wfc exited with ${result}: ${errors}")
endif()
if(DEFINED DEBUG_EXPECTED)
  file(READ "${DEBUG_EXPECTED}" debug_expected)
  string(
    REPLACE
      "
"
    "
"
    debug_expected
    "${debug_expected}"
  )
  string(
    REPLACE
      "
"
    "
"
    errors
    "${errors}"
  )
  string(STRIP "${debug_expected}" debug_expected)
  string(STRIP "${errors}" errors)
  if(NOT errors STREQUAL debug_expected)
    message(
      FATAL_ERROR
      "debug output mismatch
--- expected ---
${debug_expected}
--- actual ---
${errors}"
    )
  endif()
endif()
if(NOT actual STREQUAL expected)
  message(
    FATAL_ERROR
    "corpus output mismatch\n--- expected ---\n${expected}\n\
--- actual ---\n${actual}"
  )
endif()

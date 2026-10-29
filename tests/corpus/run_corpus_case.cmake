# Runs one corpus program through wfc and compares stdout with the expected
# text.
# The interpreter runs the program (a project is otherwise compiled).
# Usage: cmake -DWFC=<wfc> -DINPUT=<.bas|.vbp> -DEXPECTED=<file>
#   [-DDEBUG_EXPECTED=<file>] -P run_corpus_case.cmake
# DEBUG_EXPECTED holds the Immediate-window output (Debug.Print) and load
# warnings that wfc writes to stderr.
set(interpret_option --interpret)
if(PACKAGED)
  set(interpret_option "")
endif()
execute_process(
  COMMAND "${WFC}" ${interpret_option} "${INPUT}"
  OUTPUT_VARIABLE actual
  ERROR_VARIABLE errors
  RESULT_VARIABLE result
)
file(READ "${EXPECTED}" expected)
string(REPLACE "\r\n" "\n" actual "${actual}")
string(REPLACE "\r\n" "\n" expected "${expected}")
string(STRIP "${actual}" actual)
string(STRIP "${expected}" expected)
if(NOT result EQUAL 0)
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

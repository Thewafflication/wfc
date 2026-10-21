# Runs a program through the interpreter and as native machine code and
# requires the same output and the same exit code (ADR-0008: the interpreter
# is the oracle).
# Usage: cmake -DWFC=<wfc> -DPROGRAM=<.bas> -P run_native_compare.cmake
function(normalized text out)
  string(REPLACE "\r\n" "\n" text "${text}")
  string(REGEX REPLACE "[ \t\n]+$" "" text "${text}")
  set(${out} "${text}" PARENT_SCOPE)
endfunction()

execute_process(
  COMMAND "${WFC}" "${PROGRAM}"
  RESULT_VARIABLE interpreted_code
  OUTPUT_VARIABLE interpreted_output
  ERROR_VARIABLE interpreted_errors
)
execute_process(
  COMMAND "${WFC}" --native "${PROGRAM}"
  RESULT_VARIABLE native_code
  OUTPUT_VARIABLE native_output
  ERROR_VARIABLE native_errors
)
normalized("${interpreted_output}" interpreted_output)
normalized("${native_output}" native_output)
if(NOT interpreted_output STREQUAL native_output)
  string(
    CONCAT problem
    "output differs\n"
    "--- interpreter ---\n${interpreted_output}\n"
    "--- native ---\n${native_output}\n"
    "--- native errors ---\n${native_errors}"
  )
  message(FATAL_ERROR "${problem}")
endif()
if(NOT interpreted_code EQUAL native_code)
  string(
    CONCAT problem
    "exit codes differ: interpreter ${interpreted_code}, "
    "native ${native_code}\n${native_errors}"
  )
  message(FATAL_ERROR "${problem}")
endif()
# A run-time error must be the same error.
string(
  REGEX MATCH "Run-time error '[0-9]+': [^\n\r]*"
  expected_error
  "${interpreted_errors}"
)
if(NOT expected_error STREQUAL "")
  string(FIND "${native_errors}" "${expected_error}" found)
  if(found EQUAL -1)
    string(
      CONCAT problem
      "the native program did not report: ${expected_error}\n"
      "${native_errors}"
    )
    message(FATAL_ERROR "${problem}")
  endif()
endif()

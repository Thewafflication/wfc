# A program that uses a library function on Variants is refused with a
# message, not built.
# Usage: cmake -DWFC=<wfc.exe> -DWORK=<folder> -P run_refusal_test.cmake
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")
file(
  WRITE "${WORK}/Main.bas"
  "Sub Main()\n    Print Format(5, \"00\")\nEnd Sub\n"
)
file(
  WRITE "${WORK}/T.vbp"
  "Type=Exe\nModule=Main; Main.bas\nStartup=\"Sub Main\"\n"
)
execute_process(
  COMMAND "${WFC}" build "${WORK}/T.vbp" -o "${WORK}/p.exe" --target win2000
  RESULT_VARIABLE result
  ERROR_VARIABLE error
  OUTPUT_QUIET
)
if(
  NOT result EQUAL 3
  OR NOT error MATCHES "uses Variant arrays and library functions"
)
  message(FATAL_ERROR "expected a refusal (exit 3): ${result} ${error}")
endif()
if(EXISTS "${WORK}/p.exe")
  message(FATAL_ERROR "a program was written for a refused build")
endif()
message(STATUS "refused: ${error}")

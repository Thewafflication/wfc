# Runs one corpus program through wfc and compares stdout with the expected text.
# Usage: cmake -DWFC=<wfc> -DINPUT=<.bas|.vbp> -DEXPECTED=<file> -P run_corpus_case.cmake
execute_process(
    COMMAND "${WFC}" "${INPUT}"
    OUTPUT_VARIABLE actual
    ERROR_VARIABLE errors
    RESULT_VARIABLE result)
file(READ "${EXPECTED}" expected)
string(REPLACE "\r\n" "\n" actual "${actual}")
string(REPLACE "\r\n" "\n" expected "${expected}")
string(STRIP "${actual}" actual)
string(STRIP "${expected}" expected)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "wfc exited with ${result}: ${errors}")
endif()
if(NOT actual STREQUAL expected)
    message(FATAL_ERROR "corpus output mismatch\n--- expected ---\n${expected}\n--- actual ---\n${actual}")
endif()

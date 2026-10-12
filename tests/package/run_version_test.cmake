# Builds a packaged program from a project with version keys and checks the
# version resource of the executable and that it still runs.
# Usage: cmake -DWFC=<wfc> -DPROJECT=<.vbp> -DWORK=<dir> -DINFO=<script>
#   -P run_version_test.cmake
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")
set(app "${WORK}/versioned.exe")
execute_process(
  COMMAND "${WFC}" --build "${PROJECT}" -o "${app}"
  RESULT_VARIABLE built
  ERROR_VARIABLE build_errors
)
if(NOT built EQUAL 0)
  message(FATAL_ERROR "--build failed (${built}): ${build_errors}")
endif()
file(TO_NATIVE_PATH "${app}" native_app)
execute_process(
  COMMAND powershell -NoProfile -File "${INFO}" "${native_app}"
  OUTPUT_VARIABLE info
  RESULT_VARIABLE queried
)
string(STRIP "${info}" info)
string(
  JOIN "|"
  expected
  "Example Company"
  "Versioned Product"
  "Version resource test"
  "2.3.0.4"
  "Copyright 2026 Example"
  "versioned.exe"
)
if(NOT queried EQUAL 0 OR NOT info STREQUAL expected)
  message(FATAL_ERROR "version info '${info}', expected '${expected}'")
endif()
execute_process(COMMAND "${app}" OUTPUT_VARIABLE output RESULT_VARIABLE ran)
if(NOT ran EQUAL 0 OR NOT output MATCHES "versioned")
  message(FATAL_ERROR "the packaged program printed '${output}' (${ran})")
endif()

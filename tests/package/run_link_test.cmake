# A program linked dynamically (the small launcher + wfcrt.dll) prints what the
# statically linked one prints, is much smaller, and says what is missing when
# the runtime library is not there.
# Usage: cmake -DWFC=<wfc> -DPROJECT=<.vbp> -DWORK=<dir> -P run_link_test.cmake
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}/static" "${WORK}/dynamic" "${WORK}/alone")

execute_process(
  COMMAND "${WFC}" build "${PROJECT}" -o "${WORK}/static/app.exe" --native
  RESULT_VARIABLE built
  ERROR_VARIABLE errors
)
if(NOT built EQUAL 0)
  message(FATAL_ERROR "static build failed (${built}): ${errors}")
endif()
execute_process(
  COMMAND
    "${WFC}" build "${PROJECT}" -o "${WORK}/dynamic/app.exe" --native --link
    dynamic --bundle
  RESULT_VARIABLE built
  ERROR_VARIABLE errors
)
if(NOT built EQUAL 0)
  message(FATAL_ERROR "dynamic build failed (${built}): ${errors}")
endif()
if(NOT EXISTS "${WORK}/dynamic/wfcrt.dll")
  message(FATAL_ERROR "--bundle did not copy wfcrt.dll")
endif()

file(SIZE "${WORK}/static/app.exe" static_size)
file(SIZE "${WORK}/dynamic/app.exe" dynamic_size)
if(NOT dynamic_size LESS static_size)
  message(
    FATAL_ERROR
    "dynamic ${dynamic_size} is not smaller than ${static_size}"
  )
endif()

execute_process(
  COMMAND "${WORK}/static/app.exe"
  OUTPUT_VARIABLE static_out
  RESULT_VARIABLE static_code
)
execute_process(
  COMMAND "${WORK}/dynamic/app.exe"
  OUTPUT_VARIABLE dynamic_out
  RESULT_VARIABLE dynamic_code
)
if(NOT static_code EQUAL 0 OR NOT dynamic_code EQUAL 0)
  message(FATAL_ERROR "exit codes ${static_code} and ${dynamic_code}")
endif()
if(NOT static_out STREQUAL dynamic_out)
  message(FATAL_ERROR "outputs differ:\n${static_out}\n---\n${dynamic_out}")
endif()

# Without the runtime library beside it (and none installed) it says so.
file(COPY "${WORK}/dynamic/app.exe" DESTINATION "${WORK}/alone")
set(ENV{WFC_HOME} "${WORK}/nowhere")
execute_process(
  COMMAND "${WORK}/alone/app.exe"
  ERROR_VARIABLE alone_err
  RESULT_VARIABLE alone_code
)
if(alone_code EQUAL 0 OR NOT alone_err MATCHES "wfcrt.dll")
  message(
    FATAL_ERROR
    "missing runtime not reported (${alone_code}): ${alone_err}"
  )
endif()

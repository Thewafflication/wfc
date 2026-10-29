# Runs wfc with a command line and checks the exit code and the text it prints.
# Usage: cmake -DWFC=<wfc> "-DARGS=a;b;c" -DCODE=<exit code | any-failure>
#   [-DOUT_MATCHES=<regex>] [-DERR_MATCHES=<regex>] [-DSET_ENV=NAME=value]
#   -P run_cli_case.cmake
if(DEFINED SET_ENV)
  string(REGEX REPLACE "=.*" "" env_name "${SET_ENV}")
  string(REGEX REPLACE "^[^=]*=" "" env_value "${SET_ENV}")
  set(ENV{${env_name}} "${env_value}")
endif()
execute_process(
  COMMAND "${WFC}" ${ARGS}
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
  RESULT_VARIABLE result
)
if(CODE STREQUAL "any-failure")
  if(result EQUAL 0)
    message(FATAL_ERROR "expected a failure, got exit code 0\n${out}\n${err}")
  endif()
elseif(NOT result EQUAL CODE)
  message(FATAL_ERROR "exit code ${result}, expected ${CODE}\n${out}\n${err}")
endif()
if(DEFINED OUT_MATCHES AND NOT out MATCHES "${OUT_MATCHES}")
  message(FATAL_ERROR "stdout does not match '${OUT_MATCHES}':\n${out}")
endif()
if(DEFINED ERR_MATCHES AND NOT err MATCHES "${ERR_MATCHES}")
  message(FATAL_ERROR "stderr does not match '${ERR_MATCHES}':\n${err}")
endif()

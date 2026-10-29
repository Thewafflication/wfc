# add_cli_test(<name> <args> <exit code> [-DOUT_MATCHES=<regex>]
#   [-DERR_MATCHES=<regex>]): runs wfc with the arguments (a list) and checks
# the exit code and what it prints (tests/cli/run_cli_case.cmake).
function(add_cli_test name args code)
  add_test(
    NAME TC-MP0010-cli-${name}
    COMMAND
      "${CMAKE_COMMAND}" "-DWFC=$<TARGET_FILE:wfc>" "-DARGS=${args}"
      "-DCODE=${code}" ${ARGN} -P
      "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/run_cli_case.cmake"
  )
  set_tests_properties(
    TC-MP0010-cli-${name}
    PROPERTIES LABELS "integration;mp-0010;cli"
  )
endfunction()

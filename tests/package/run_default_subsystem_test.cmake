# A project with forms builds, with no option, a Windows-subsystem program (as
# VB6 makes every program); --console keeps the console program.
# Usage: cmake -DWFC=<wfc> -DPROJECT=<.vbp> -DWORK=<dir>
#   -P run_default_subsystem_test.cmake
function(subsystem_of file result)
  file(READ "${file}" header LIMIT 4096 HEX)
  string(SUBSTRING "${header}" 120 8 lfanew_hex)
  string(REGEX MATCHALL ".." lfanew_bytes "${lfanew_hex}")
  list(REVERSE lfanew_bytes)
  string(JOIN "" lfanew_text ${lfanew_bytes})
  math(EXPR lfanew "0x${lfanew_text}")
  math(EXPR at "(${lfanew} + 24 + 68) * 2")
  string(SUBSTRING "${header}" ${at} 4 value)
  set(${result} "${value}" PARENT_SCOPE)
endfunction()

file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")
foreach(case default console)
  set(options "")
  if(case STREQUAL "console")
    set(options --console)
  endif()
  execute_process(
    COMMAND "${WFC}" --build "${PROJECT}" -o "${WORK}/${case}.exe" ${options}
    RESULT_VARIABLE built
    ERROR_VARIABLE build_errors
  )
  if(NOT built EQUAL 0)
    message(FATAL_ERROR "--build (${case}) failed (${built}): ${build_errors}")
  endif()
  subsystem_of("${WORK}/${case}.exe" subsystem)
  if(case STREQUAL "default" AND NOT subsystem STREQUAL "0200")
    message(FATAL_ERROR "default build has subsystem bytes ${subsystem}")
  endif()
  if(case STREQUAL "console" AND NOT subsystem STREQUAL "0300")
    message(FATAL_ERROR "--console build has subsystem bytes ${subsystem}")
  endif()
endforeach()

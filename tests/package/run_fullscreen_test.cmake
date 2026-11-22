# --fullscreen on a build makes a Windows-subsystem program that carries the
# option, so it runs scaled to the screen like `wfc run --fullscreen`.
# Usage: cmake -DWFC=<wfc> -DPROJECT=<.vbp> -DWORK=<dir>
#   -P run_fullscreen_test.cmake
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")
foreach(case plain fullscreen)
  set(options "")
  if(case STREQUAL "fullscreen")
    set(options --fullscreen)
  endif()
  execute_process(
    COMMAND "${WFC}" --build "${PROJECT}" -o "${WORK}/${case}.exe" ${options}
    RESULT_VARIABLE built
    ERROR_VARIABLE build_errors
  )
  if(NOT built EQUAL 0)
    message(FATAL_ERROR "--build (${case}) failed (${built}): ${build_errors}")
  endif()
  # The option sits in the payload at the end of the program.
  file(SIZE "${WORK}/${case}.exe" size)
  math(EXPR start "${size} - 200000")
  if(start LESS 0)
    set(start 0)
  endif()
  file(READ "${WORK}/${case}.exe" tail OFFSET ${start} HEX)
  # "--fullscreen" in hex
  string(FIND "${tail}" "2d2d66756c6c73637265656e" found)
  if(case STREQUAL "fullscreen" AND found EQUAL -1)
    message(FATAL_ERROR "the --fullscreen build does not carry the option")
  endif()
  if(case STREQUAL "plain" AND NOT found EQUAL -1)
    message(FATAL_ERROR "a build without --fullscreen carries the option")
  endif()
endforeach()

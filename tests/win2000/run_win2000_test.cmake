# Builds every program of a folder for Windows 2000 (`wfc build --target
# win2000`), runs it, and checks that its output, error output and exit code
# are those of the same program compiled for this machine. A program that uses
# a feature the small runtime lacks is refused at build time (exit code 3 and a
# message); that is counted, not a failure.
# Usage: cmake -DWFC=<wfc.exe> -DPROGRAMS=<folder of .bas files>
#   -DWORK=<folder> -DMINIMUM=<least number of programs that must run>
#   -P run_win2000_test.cmake
file(GLOB programs "${PROGRAMS}/*.bas")
set(passed 0)
set(refused 0)
foreach(program IN LISTS programs)
  get_filename_component(name "${program}" NAME_WE)
  set(directory "${WORK}/${name}")
  file(REMOVE_RECURSE "${directory}")
  file(MAKE_DIRECTORY "${directory}")
  file(COPY_FILE "${program}" "${directory}/Main.bas")
  file(
    WRITE "${directory}/T.vbp"
    "Type=Exe\nModule=Main; Main.bas\nStartup=\"Sub Main\"\n"
  )
  execute_process(
    COMMAND
      "${WFC}" build "${directory}/T.vbp" -o "${directory}/p.exe" --target
      win2000
    RESULT_VARIABLE built
    ERROR_VARIABLE build_error
    OUTPUT_QUIET
  )
  if(NOT built EQUAL 0)
    if(built EQUAL 3 AND build_error MATCHES "does not have yet")
      math(EXPR refused "${refused} + 1")
      continue()
    endif()
    message(FATAL_ERROR "${name}: the build failed (${built}): ${build_error}")
  endif()
  execute_process(
    COMMAND "${WFC}" "${directory}/T.vbp"
    RESULT_VARIABLE expected_code
    OUTPUT_VARIABLE expected_output
    ERROR_VARIABLE expected_error
    TIMEOUT 60
  )
  execute_process(
    COMMAND "${directory}/p.exe"
    RESULT_VARIABLE actual_code
    OUTPUT_VARIABLE actual_output
    ERROR_VARIABLE actual_error
    TIMEOUT 60
  )
  if(NOT "${actual_output}" STREQUAL "${expected_output}")
    message(
      FATAL_ERROR
      "${name}: output differs\n"
      "expected:\n${expected_output}\ngot:\n${actual_output}"
    )
  endif()
  if(NOT "${actual_error}" STREQUAL "${expected_error}")
    message(
      FATAL_ERROR
      "${name}: error output differs\n"
      "expected: ${expected_error}\ngot: ${actual_error}"
    )
  endif()
  if(NOT "${actual_code}" STREQUAL "${expected_code}")
    message(
      FATAL_ERROR
      "${name}: exit code ${actual_code}, expected ${expected_code}"
    )
  endif()
  math(EXPR passed "${passed} + 1")
endforeach()
if(passed LESS MINIMUM)
  message(
    FATAL_ERROR
    "only ${passed} programs ran (at least ${MINIMUM} expected), "
    "${refused} refused"
  )
endif()
message(
  STATUS
  "${passed} programs run like the native ones; ${refused} refused"
)

# Runs the Select Case probe under WFC and compares what it measured with the
# answer of Visual Basic 6.
#
# Usage: cmake -DWFC=<wfc> -DPWSH=<pwsh> -DGENERATOR=<New-SelectCaseProbe.ps1>
#   -DREFERENCE=<vb6-reference.txt> -DWORK=<folder> -DMODE=interpret|native
#   -P run_probe.cmake
#
# Every line of the reference must be reproduced, including those of the module
# that has `Option Compare Text` (the option belongs to one module) and the one
# where `On Error Resume Next` goes on in the Case body after an error.
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")
execute_process(
  COMMAND "${PWSH}" -NoProfile -File "${GENERATOR}" -Output "${WORK}"
  RESULT_VARIABLE generated
  OUTPUT_QUIET
)
if(NOT generated EQUAL 0)
  message(FATAL_ERROR "the generator failed (${generated})")
endif()
set(interpret_option "")
if(MODE STREQUAL "interpret")
  set(interpret_option --interpret)
endif()
execute_process(
  COMMAND "${WFC}" ${interpret_option} "${WORK}/SelectCaseProbe.vbp"
  RESULT_VARIABLE ran
  OUTPUT_VARIABLE output
  ERROR_VARIABLE errors
)
if(NOT ran EQUAL 0)
  message(FATAL_ERROR "wfc exited with ${ran}: ${errors}${output}")
endif()
file(STRINGS "${WORK}/select-case-probe.txt" measured)
file(STRINGS "${REFERENCE}" wanted)
list(LENGTH wanted wanted_count)
list(LENGTH measured measured_count)
if(NOT wanted_count EQUAL measured_count)
  message(
    FATAL_ERROR
    "the probe wrote ${measured_count} lines, "
    "Visual Basic wrote ${wanted_count}"
  )
endif()
set(different 0)
set(report "")
math(EXPR last "${wanted_count} - 1")
foreach(index RANGE ${last})
  list(GET wanted ${index} want)
  list(GET measured ${index} have)
  if(NOT want STREQUAL have)
    math(EXPR different "${different} + 1")
    if(different LESS_EQUAL 10)
      string(APPEND report "\n  Visual Basic: ${want}\n  WFC:          ${have}")
    endif()
  endif()
endforeach()
if(different GREATER 0)
  message(
    FATAL_ERROR
    "${different} of ${wanted_count} lines differ from Visual Basic 6:${report}"
  )
endif()
message(STATUS "all ${wanted_count} lines are those of Visual Basic 6")

# Runs the Select Case probe under WFC and compares what it measured with the
# answer of Visual Basic 6.
#
# Usage: cmake -DWFC=<wfc> -DPWSH=<pwsh> -DGENERATOR=<New-SelectCaseProbe.ps1>
#   -DREFERENCE=<vb6-reference.txt> -DWORK=<folder> -DMODE=interpret|native
#   -P run_probe.cmake
#
# The generator writes the project without the module that has
# `Option Compare Text`, which WFC applies to the whole program (Visual Basic
# to one module), so the reference lines of that module are left out. One
# more line is left out: after an error in a Case value, `On Error Resume
# Next` continues in the Case body in Visual Basic and after the Select Case in
# WFC (the line "an error in a value that is reached").
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")
execute_process(
  COMMAND
    "${PWSH}" -NoProfile -File "${GENERATOR}" -Output "${WORK}" -WithoutText
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
file(STRINGS "${WORK}/select-case-probe.txt" got)
file(STRINGS "${REFERENCE}" reference)
set(wanted "")
foreach(line IN LISTS reference)
  if(
    NOT line MATCHES "^Option Compare Text"
    AND NOT line MATCHES "^flow \| an error in a value that is reached"
  )
    list(APPEND wanted "${line}")
  endif()
endforeach()
set(measured "")
foreach(line IN LISTS got)
  if(NOT line MATCHES "^flow \| an error in a value that is reached")
    list(APPEND measured "${line}")
  endif()
endforeach()
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

# The small runtime is the default target of an x86 wfc, and of any wfc with
# WFC_TARGET=win2000: a program it can run is written on wfcw2k.exe (small), a
# program that needs more gets the runtime of this wfc with a note, and
# `--target host` always takes the runtime of this wfc.
# Usage: cmake -DWFC=<wfc.exe> -DWORK=<folder> -P run_default_test.cmake
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}/simple" "${WORK}/library")
file(WRITE "${WORK}/simple/Main.bas" "Sub Main()\n    Print \"hi\"\nEnd Sub\n")
file(
  WRITE "${WORK}/library/Main.bas"
  "Sub Main()\n    Print Format(5, \"00\")\nEnd Sub\n"
)
foreach(name simple library)
  file(
    WRITE "${WORK}/${name}/T.vbp"
    "Type=Exe\nModule=Main; Main.bas\nStartup=\"Sub Main\"\n"
  )
endforeach()
set(ENV{WFC_TARGET} win2000)

function(build name output)
  execute_process(
    COMMAND
      "${WFC}" build "${WORK}/${name}/T.vbp" -o "${WORK}/${output}" ${ARGN}
    RESULT_VARIABLE result
    ERROR_VARIABLE error
    OUTPUT_QUIET
  )
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "building ${name} failed (${result}): ${error}")
  endif()
  set(build_error "${error}" PARENT_SCOPE)
  file(SIZE "${WORK}/${output}" size)
  set(build_size ${size} PARENT_SCOPE)
endfunction()

# A program the small runtime can run: small, and it runs.
build(simple small.exe)
if(build_size GREATER 400000)
  message(FATAL_ERROR "the default build is ${build_size} bytes, not small")
endif()
execute_process(
  COMMAND "${WORK}/small.exe"
  OUTPUT_VARIABLE printed
  RESULT_VARIABLE ran
)
if(NOT ran EQUAL 0 OR NOT printed MATCHES "^hi")
  message(FATAL_ERROR "the small program printed '${printed}' (${ran})")
endif()

# A program with a library function on Variants: the runtime of this wfc,
# with a note.
build(library full.exe)
if(NOT build_error MATCHES "note: the program uses the library function")
  message(FATAL_ERROR "no note about the function: '${build_error}'")
endif()
if(build_size LESS 1000000)
  message(FATAL_ERROR "the library program is ${build_size} bytes, not full")
endif()

# --target host: the runtime of this wfc, without a note.
build(simple host.exe --target host)
if(build_size LESS 1000000 OR NOT build_error STREQUAL "")
  message(FATAL_ERROR "--target host: ${build_size} bytes, '${build_error}'")
endif()
message(STATUS "default target: small, fallback with a note, host on request")

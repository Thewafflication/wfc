# Builds a packaged program from a corpus project and checks that it prints what
# the plain run prints, with the project directory removed from its reach.
# Usage: cmake -DWFC=<wfc> -DINPUT=<.vbp> -DEXPECTED=<file> -DWORK=<dir>
#   [-DDEBUG_EXPECTED=<file>] -P run_package_case.cmake
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")
set(app "${WORK}/app${CMAKE_EXECUTABLE_SUFFIX}")
execute_process(
  COMMAND "${WFC}" --build "${INPUT}" -o "${app}" --interpret
  RESULT_VARIABLE built
  ERROR_VARIABLE build_errors
)
if(NOT built EQUAL 0)
  message(FATAL_ERROR "--build failed (${built}): ${build_errors}")
endif()
if(NOT EXISTS "${app}")
  message(FATAL_ERROR "--build did not write ${app}")
endif()
set(WFC "${app}")
set(PACKAGED TRUE)
include("${CMAKE_CURRENT_LIST_DIR}/../corpus/run_corpus_case.cmake")

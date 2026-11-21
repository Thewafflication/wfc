# A packaged native program finds its own files through App.Path, not the
# folder it was built from (which is gone, or elsewhere, on another machine).
# Usage: cmake -DWFC=<wfc> -DPROJECT=<.vbp> -DWORK=<dir>
#   -P run_app_path_test.cmake
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}/src")
get_filename_component(project_dir "${PROJECT}" DIRECTORY)
file(COPY "${project_dir}/" DESTINATION "${WORK}/src")
get_filename_component(project_name "${PROJECT}" NAME)
execute_process(
  COMMAND
    "${WFC}" --build "${WORK}/src/${project_name}" -o "${WORK}/app.exe"
    --console
  RESULT_VARIABLE built
  ERROR_VARIABLE build_errors
)
if(NOT built EQUAL 0)
  message(FATAL_ERROR "--build failed (${built}): ${build_errors}")
endif()
# The folder it was built from goes away.
file(REMOVE_RECURSE "${WORK}/src")
execute_process(
  COMMAND "${WORK}/app.exe"
  RESULT_VARIABLE ran
  OUTPUT_VARIABLE output
  ERROR_VARIABLE errors
)
if(NOT output MATCHES "reads True")
  message(
    FATAL_ERROR
    "App.Path found no files (${ran}):
${output}${errors}"
  )
endif()

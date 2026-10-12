# Builds a packaged program from a project whose IconForm has an icon and
# checks that the executable shows that icon rather than the runtime's own.
# Usage: cmake -DWFC=<wfc> -DPROJECT=<.vbp> -DWORK=<dir> -DCHECK=<script>
#   -P run_icon_test.cmake
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")
set(app "${WORK}/iconed.exe")
execute_process(
  COMMAND "${WFC}" --build "${PROJECT}" -o "${app}"
  RESULT_VARIABLE built
  ERROR_VARIABLE build_errors
)
if(NOT built EQUAL 0)
  message(FATAL_ERROR "--build failed (${built}): ${build_errors}")
endif()
file(TO_NATIVE_PATH "${app}" native_app)
file(TO_NATIVE_PATH "${WFC}" native_wfc)
execute_process(
  COMMAND powershell -NoProfile -File "${CHECK}" "${native_app}" "${native_wfc}"
  RESULT_VARIABLE differs
  ERROR_VARIABLE reason
)
if(NOT differs EQUAL 0)
  message(FATAL_ERROR "icon check failed: ${reason}")
endif()

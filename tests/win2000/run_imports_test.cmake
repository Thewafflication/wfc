# Checks that programs import nothing newer than Windows 2000 (WCRT's gate with
# the allowlist of WFC's runtime for Windows 2000).
# Usage: cmake -DPOWERSHELL=<pwsh>
#   -DGATE=<wcrt/tools/test-windows-2000-imports.ps1>
#   -DALLOWLIST=<json file> -DBINARIES=<exe;exe> -DWORK=<folder> -P ...
# The gate takes the allowlist and output folder relative to the WCRT root.
get_filename_component(tools "${GATE}" DIRECTORY)
get_filename_component(wcrt_root "${tools}" DIRECTORY)
file(RELATIVE_PATH allowlist "${wcrt_root}" "${ALLOWLIST}")
file(RELATIVE_PATH results "${wcrt_root}" "${WORK}")
foreach(binary IN LISTS BINARIES)
  execute_process(
    COMMAND
      "${POWERSHELL}" -NoProfile -File "${GATE}" -Binary "${binary}" -Allowlist
      "${allowlist}" -OutputRoot "${results}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
  )
  if(NOT result EQUAL 0)
    message(
      FATAL_ERROR
      "${binary} imports more than Windows 2000 has:\n${output}\n${error}"
    )
  endif()
endforeach()
message(STATUS "imports of ${BINARIES} are all in Windows 2000")

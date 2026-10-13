# Registers the screenshot test of one corpus game (included by the top-level
# CMakeLists.txt, which sets vb6games_dir and project_checks).
function(add_vb6_game name vbp forms milliseconds)
  if(EXISTS "${vb6games_dir}/${vbp}")
    add_test(
      NAME TC-MP0008-vb6games-${name}
      COMMAND
        "${CMAKE_COMMAND}" "-DWFC=$<TARGET_FILE:wfc>"
        "-DPROJECT=${vb6games_dir}/${vbp}"
        "-DSHOTS=${CMAKE_BINARY_DIR}/screenshots/${name}" "-DFORMS=${forms}"
        "-DMILLISECONDS=${milliseconds}" -P
        "${project_checks}/run_project_screenshots.cmake"
    )
    set_tests_properties(
      TC-MP0008-vb6games-${name}
      PROPERTIES
        RESOURCE_LOCK "desktop"
        LABELS "integration;windows;mp-0008;vb6games;evidence"
        TIMEOUT 90
    )
  endif()
endfunction()

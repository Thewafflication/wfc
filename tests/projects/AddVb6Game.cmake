# Registers the screenshot test of one corpus game (included by the top-level
# CMakeLists.txt, which sets vb6games_dir and project_checks).
# Optional fifth and sixth arguments: a ui-script that drives the windows and
# the screenshot file it must take.
function(add_vb6_game name vbp forms milliseconds)
  if(EXISTS "${vb6games_dir}/${vbp}")
    set(script_arguments "")
    if(ARGC GREATER 5)
      set(
        script_arguments
        "-DSCRIPT=${project_checks}/${ARGV4}"
        "-DFILES=${ARGV5}"
      )
    endif()
    add_test(
      NAME TC-MP0008-vb6games-${name}
      COMMAND
        "${CMAKE_COMMAND}" "-DWFC=$<TARGET_FILE:wfc>"
        "-DPROJECT=${vb6games_dir}/${vbp}"
        "-DSHOTS=${CMAKE_BINARY_DIR}/screenshots/${name}" "-DFORMS=${forms}"
        "-DMILLISECONDS=${milliseconds}" ${script_arguments} -P
        "${project_checks}/run_project_screenshots.cmake"
    )
    set_tests_properties(
      TC-MP0008-vb6games-${name}
      PROPERTIES
        RESOURCE_LOCK "desktop"
        LABELS "integration;windows;mp-0008;vb6games;evidence"
        TIMEOUT 240
    )
  endif()
endfunction()

# Registers a scripted play session of a corpus game: the headless program
# runs the script, whose `get`, `messages` lines state what it expects.
# An optional fifth argument is the WFC_RANDOM_SEED of the session.
function(add_vb6_play name vbp script expected)
  if(EXISTS "${vb6games_dir}/${vbp}")
    set(seed_argument "")
    if(ARGC GREATER 4)
      set(seed_argument "-DSEED=${ARGV4}")
    endif()
    add_test(
      NAME TC-MP0008-vb6games-${name}
      COMMAND
        "${CMAKE_COMMAND}" "-DWFC=$<TARGET_FILE:wfc>"
        "-DPROJECT=${vb6games_dir}/${vbp}"
        "-DSCRIPT=${project_checks}/${script}"
        "-DEXPECTED=${project_checks}/${expected}"
        "-DWORK=${CMAKE_BINARY_DIR}/vb6games-${name}" ${seed_argument} -P
        "${project_checks}/run_project_script.cmake"
    )
    set_tests_properties(
      TC-MP0008-vb6games-${name}
      PROPERTIES LABELS "integration;mp-0008;vb6games" TIMEOUT 120
    )
  endif()
endfunction()

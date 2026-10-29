# Registers the screenshot test of one corpus game (included by the top-level
# CMakeLists.txt, which sets vb6games_dir and project_checks).
# Optional fifth and sixth arguments: a ui-script that drives the windows and
# the screenshot file it must take; an optional seventh, the WFC_RANDOM_SEED of
# the run (a game that plays at random must not end before the screenshot).
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
    if(ARGC GREATER 6)
      list(APPEND script_arguments "-DSEED=${ARGV6}")
    endif()
    # Games run as machine code (MP-0009). SuMBOku 1.4.2 assigns an object
    # without Set (`Dest = Me.Value`), which only the interpreter accepts.
    set(native_argument "-DNATIVE=1")
    if("${name}" STREQUAL "sumboku")
      set(native_argument "-DNATIVE=0")
    endif()
    add_test(
      NAME TC-MP0008-vb6games-${name}
      COMMAND
        "${CMAKE_COMMAND}" "-DWFC=$<TARGET_FILE:wfc>"
        "-DPROJECT=${vb6games_dir}/${vbp}"
        "-DSHOTS=${CMAKE_BINARY_DIR}/screenshots/${name}" "-DFORMS=${forms}"
        "-DMILLISECONDS=${milliseconds}" ${native_argument} ${script_arguments}
        -P "${project_checks}/run_project_screenshots.cmake"
    )
    set_tests_properties(
      TC-MP0008-vb6games-${name}
      PROPERTIES
        RESOURCE_LOCK "desktop"
        LABELS "integration;windows;mp-0008;vb6games;evidence"
        TIMEOUT 600
    )
  endif()
endfunction()

# Registers a scripted play session of a corpus game: the headless program
# runs the script, whose `get`, `messages` lines state what it expects.
# An optional fifth argument is the WFC_RANDOM_SEED of the session, an optional
# sixth the screen size "WIDTHxHEIGHT" the program sees (WFC_SCREEN), so a
# layout that follows the screen does not depend on the machine's display.
function(add_vb6_play name vbp script expected)
  # The optional seed may be empty, which an unquoted ${ARGN} would drop.
  if(ARGC GREATER 5)
    add_vb6_play_with(
      ""
      ${name}
      ${vbp}
      ${script}
      ${expected}
      "${ARGV4}"
      "${ARGV5}"
    )
  elseif(ARGC GREATER 4)
    add_vb6_play_with("" ${name} ${vbp} ${script} ${expected} "${ARGV4}")
  else()
    add_vb6_play_with("" ${name} ${vbp} ${script} ${expected})
  endif()
endfunction()

# The same session with the program compiled to machine code (MP-0009).
function(add_vb6_native_play name vbp script expected)
  # The optional seed may be empty, which an unquoted ${ARGN} would drop.
  if(ARGC GREATER 5)
    add_vb6_play_with(
      "native"
      ${name}
      ${vbp}
      ${script}
      ${expected}
      "${ARGV4}"
      "${ARGV5}"
    )
  elseif(ARGC GREATER 4)
    add_vb6_play_with("native" ${name} ${vbp} ${script} ${expected} "${ARGV4}")
  else()
    add_vb6_play_with("native" ${name} ${vbp} ${script} ${expected})
  endif()
endfunction()

function(
  add_vb6_play_with
  mode
  name
  vbp
  script
  expected
)
  set(native_argument "")
  set(test_name TC-MP0008-vb6games-${name})
  set(test_labels "integration;mp-0008;vb6games")
  if(NOT "${mode}" STREQUAL "")
    set(native_argument "-DNATIVE=1")
    set(test_name TC-MP0009-native-vb6games-${name})
    set(test_labels "integration;mp-0009;native;vb6games")
  endif()
  if(EXISTS "${vb6games_dir}/${vbp}")
    set(seed_argument "")
    if(ARGC GREATER 5 AND NOT "${ARGV5}" STREQUAL "")
      set(seed_argument "-DSEED=${ARGV5}")
    endif()
    set(screen_argument "")
    if(ARGC GREATER 6)
      set(screen_argument "-DSCREEN=${ARGV6}")
    endif()
    add_test(
      NAME ${test_name}
      COMMAND
        "${CMAKE_COMMAND}" "-DWFC=$<TARGET_FILE:wfc>"
        "-DPROJECT=${vb6games_dir}/${vbp}"
        "-DSCRIPT=${project_checks}/${script}"
        "-DEXPECTED=${project_checks}/${expected}"
        "-DWORK=${CMAKE_BINARY_DIR}/vb6games-${mode}${name}" ${seed_argument}
        ${screen_argument} ${native_argument} -P
        "${project_checks}/run_project_script.cmake"
    )
    set_tests_properties(
      ${test_name}
      PROPERTIES LABELS "${test_labels}" TIMEOUT 300
    )
    # Capture King listens on a UDP port.
    if("${name}" MATCHES "captureking")
      set_tests_properties(${test_name} PROPERTIES RESOURCE_LOCK udp)
    endif()
  endif()
endfunction()

# Registers the packaged-program test of one corpus game: `wfc --build` makes
# a standalone program, which runs on its own and shows the named form. An
# optional fourth argument is the WFC_RANDOM_SEED of the run.
function(add_vb6_package name vbp form)
  if(EXISTS "${vb6games_dir}/${vbp}")
    set(seed_argument "")
    if(ARGC GREATER 3)
      set(seed_argument "-DSEED=${ARGV3}")
    endif()
    add_test(
      NAME TC-MP0008-package-${name}
      COMMAND
        "${CMAKE_COMMAND}" "-DWFC=$<TARGET_FILE:wfc>"
        "-DPROJECT=${vb6games_dir}/${vbp}"
        "-DWORK=${CMAKE_BINARY_DIR}/package-${name}" "-DFORM=${form}"
        ${seed_argument} -P
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/package/run_package_gui.cmake"
    )
    set_tests_properties(
      TC-MP0008-package-${name}
      PROPERTIES
        RESOURCE_LOCK "desktop"
        LABELS "integration;windows;mp-0008;package"
        TIMEOUT 600
    )
  endif()
endfunction()

# The same packaged-program test with the program compiled to machine code
# (MP-0009): the packaged program carries the code, not a program to read.
function(add_vb6_native_package name vbp form)
  if(EXISTS "${vb6games_dir}/${vbp}")
    set(seed_argument "")
    if(ARGC GREATER 3)
      set(seed_argument "-DSEED=${ARGV3}")
    endif()
    add_test(
      NAME TC-MP0009-native-package-${name}
      COMMAND
        "${CMAKE_COMMAND}" "-DWFC=$<TARGET_FILE:wfc>"
        "-DPROJECT=${vb6games_dir}/${vbp}"
        "-DWORK=${CMAKE_BINARY_DIR}/package-native-${name}" "-DFORM=${form}"
        "-DNATIVE=1" ${seed_argument} -P
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/package/run_package_gui.cmake"
    )
    set_tests_properties(
      TC-MP0009-native-package-${name}
      PROPERTIES
        RESOURCE_LOCK "desktop"
        LABELS "integration;windows;mp-0009;native;package"
        TIMEOUT 600
    )
  endif()
endfunction()

# The same with the program linked dynamically (MP-0010): a small launcher
# program that loads wfcrt.dll, bundled beside it.
function(add_vb6_dynamic_package name vbp form)
  if(EXISTS "${vb6games_dir}/${vbp}")
    set(seed_argument "")
    if(ARGC GREATER 3)
      set(seed_argument "-DSEED=${ARGV3}")
    endif()
    add_test(
      NAME TC-MP0010-dynamic-package-${name}
      COMMAND
        "${CMAKE_COMMAND}" "-DWFC=$<TARGET_FILE:wfc>"
        "-DPROJECT=${vb6games_dir}/${vbp}"
        "-DWORK=${CMAKE_BINARY_DIR}/package-dynamic-${name}" "-DFORM=${form}"
        "-DNATIVE=1" "-DLINK=dynamic" ${seed_argument} -P
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/package/run_package_gui.cmake"
    )
    set_tests_properties(
      TC-MP0010-dynamic-package-${name}
      PROPERTIES
        RESOURCE_LOCK "desktop"
        LABELS "integration;windows;mp-0010;native;package"
        TIMEOUT 600
    )
  endif()
endfunction()

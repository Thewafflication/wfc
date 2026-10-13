# Plays one complete Capture King game of random moves and chat messages to
# its conclusion, headless: pieces are dragged to random squares (favouring
# captures) until a king is taken, the player who took it calls check mate,
# and the board the program saved must be the board this script computed.
# Usage: cmake -DWFC=<wfc> -DPROJECT=<CaptureKing.vbp> -DWORK=<dir>
#   [-DSEED=<n>] [-DMAX_PLIES=<n>] -P run_captureking_game.cmake
if(NOT DEFINED SEED)
  set(SEED 20261012)
endif()
if(NOT DEFINED MAX_PLIES)
  set(MAX_PLIES 600)
endif()

# A small linear congruential generator, so every platform plays the same game.
set(rng_state ${SEED})
function(next_random limit out)
  math(EXPR rng_state "(${rng_state} * 1103515245 + 12345) % 2147483648")
  set(rng_state ${rng_state} PARENT_SCOPE)
  math(EXPR value "(${rng_state} / 65536) % ${limit}")
  set(${out} ${value} PARENT_SCOPE)
endfunction()

# The starting position, as the game's SetBoardState has it.
set(
  black_back
  BR1
  BK1
  BB1
  B+1
  BQ1
  BB2
  BK2
  BR2
)
set(
  white_back
  WR1
  WK1
  WB1
  WQ1
  W+1
  WB2
  WK2
  WR2
)
foreach(column RANGE 0 7)
  list(GET black_back ${column} piece)
  set(board_${column} "${piece}")
  list(GET white_back ${column} piece)
  math(EXPR square "56 + ${column}")
  set(board_${square} "${piece}")
  math(EXPR number "${column} + 1")
  math(EXPR square "8 + ${column}")
  set(board_${square} "BP${number}")
  math(EXPR square "48 + ${column}")
  set(board_${square} "WP${number}")
endforeach()
foreach(square RANGE 16 47)
  set(board_${square} "")
endforeach()

set(
  chat
  "good luck"
  "nice move"
  "I did not see that"
  "your move"
  "think carefully now"
  "that was a gift"
  "well played"
  "hmm"
  "no taking back"
  "I am winning"
)
list(LENGTH chat chat_count)

set(script "wait 3500\nclick frmWelcome.picLocalGame\nwait 300\n")
set(turn W)
set(plies 0)
set(winner "")
while(plies LESS ${MAX_PLIES} AND winner STREQUAL "")
  if(turn STREQUAL "W")
    set(opponent B)
  else()
    set(opponent W)
  endif()
  set(own "")
  set(enemy "")
  foreach(square RANGE 0 63)
    set(piece "${board_${square}}")
    if(NOT piece STREQUAL "")
      string(SUBSTRING "${piece}" 0 1 colour)
      if(colour STREQUAL turn)
        # Pawns that would reach the last rank open the promotion form: skip.
        list(APPEND own ${square})
      else()
        list(APPEND enemy ${square})
      endif()
    endif()
  endforeach()
  list(LENGTH own own_count)
  list(LENGTH enemy enemy_count)
  if(own_count EQUAL 0)
    break()
  endif()
  set(from -1)
  set(to -1)
  set(attempts 0)
  while(to LESS 0 AND attempts LESS 200)
    math(EXPR attempts "${attempts} + 1")
    next_random(${own_count} pick)
    list(GET own ${pick} candidate)
    set(piece "${board_${candidate}}")
    string(SUBSTRING "${piece}" 1 1 kind)
    next_random(100 mode)
    if(mode LESS 12 AND enemy_count GREATER 0)
      next_random(${enemy_count} pick)
      list(GET enemy ${pick} target)
    else()
      next_random(64 target)
      set(there "${board_${target}}")
      string(SUBSTRING "${there}" 0 1 there_colour)
      if(there_colour STREQUAL turn)
        continue()
      endif()
    endif()
    if(target EQUAL candidate)
      continue()
    endif()
    # Pawns never reach the far rank (it opens the promotion form).
    if(kind STREQUAL "P" AND (target LESS 8 OR target GREATER 54))
      continue()
    endif()
    set(from ${candidate})
    set(to ${target})
  endwhile()
  if(to LESS 0)
    break()
  endif()
  set(captured "${board_${to}}")
  string(
    APPEND script
    "mousedown frmGame.picboard(${from}) 1 0 10 10\n"
    "drag frmGame.picboard(${from}) frmGame.picboard(${to}) 10 10\n"
    "wait 20\n"
  )
  set(board_${to} "${board_${from}}")
  set(board_${from} "")
  math(EXPR plies "${plies} + 1")
  # A message every third move.
  math(EXPR chat_turn "${plies} % 3")
  if(chat_turn EQUAL 0)
    next_random(${chat_count} message_index)
    list(GET chat ${message_index} message)
    string(
      APPEND script
      "type frmGame.txtMessageOut ${message}\n"
      "click frmGame.cmdSend\nwait 20\n"
    )
  endif()
  if(NOT captured STREQUAL "")
    string(SUBSTRING "${captured}" 1 1 captured_kind)
    if(captured_kind STREQUAL "+")
      set(winner ${turn})
    endif()
  endif()
  set(turn ${opponent})
endwhile()
if(winner STREQUAL "")
  message(FATAL_ERROR "no king was taken in ${MAX_PLIES} moves (seed ${SEED})")
endif()
message(STATUS "seed ${SEED}: ${winner} took the king after ${plies} moves")

# The winner calls check mate and confirms the question.
string(APPEND script "answer msgbox 6\nclick frmGame.picCheckMate\nwait 200\n")

# The board the program should have saved last, one square per line (CMake
# reads the file with plain line feeds).
set(expected "")
foreach(square RANGE 0 63)
  string(APPEND expected "${board_${square}}\n")
endforeach()

file(REMOVE_RECURSE "${WORK}")
get_filename_component(project_dir "${PROJECT}" DIRECTORY)
get_filename_component(project_file "${PROJECT}" NAME)
file(COPY "${project_dir}/" DESTINATION "${WORK}")
# The program deletes its saved game when it unloads; keep it for the check.
file(READ "${WORK}/CaptureKing.frm" frm_text)
string(
  REPLACE "Kill App.Path & \"\\game.tmp\""
  "'Kill game.tmp"
  frm_text
  "${frm_text}"
)
file(WRITE "${WORK}/CaptureKing.frm" "${frm_text}")
file(WRITE "${WORK}-game.ui" "${script}")
execute_process(
  COMMAND "${WFC}" "${WORK}/${project_file}" --ui-script "${WORK}-game.ui"
  RESULT_VARIABLE result
  ERROR_VARIABLE errors
  OUTPUT_VARIABLE output
)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "wfc exited with ${result}: ${errors}")
endif()
if(NOT "${errors}" STREQUAL "")
  message(FATAL_ERROR "unexpected diagnostics: ${errors}")
endif()
file(READ "${WORK}/game.tmp" saved)
string(LENGTH "${expected}" expected_length)
string(LENGTH "${saved}" saved_length)
if(saved_length LESS expected_length)
  message(FATAL_ERROR "game.tmp is too short (${saved_length} bytes)")
endif()
math(EXPR tail_start "${saved_length} - ${expected_length}")
string(SUBSTRING "${saved}" ${tail_start} ${expected_length} tail)
if(NOT tail STREQUAL expected)
  file(WRITE "${WORK}-expected.txt" "${expected}")
  message(
    FATAL_ERROR
    "the saved board differs: see ${WORK}/game.tmp, ${WORK}-expected.txt"
  )
endif()
message(STATUS "the saved board matches after ${plies} moves")

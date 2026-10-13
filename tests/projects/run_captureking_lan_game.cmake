# Plays one complete Capture King game between two programs on a virtual LAN
# (two WFC processes, each with its own loopback address and user name):
# bravo invites alpha, alpha accepts and plays white; random moves and chat
# messages travel over real UDP sockets until a king is taken, the winner
# calls check mate and the loser confirms. Both programs must have saved the
# board this script computed, and the winner's statistics must show the win.
# Usage: cmake -DWFC=<wfc> -DPROJECT=<CaptureKing.vbp> -DWORK=<dir>
#   [-DSEED=<n>] [-DMAX_PLIES=<n>] -P run_captureking_lan_game.cmake
if(NOT DEFINED SEED)
  set(SEED 99)
endif()
if(NOT DEFINED MAX_PLIES)
  set(MAX_PLIES 300)
endif()
# The game saves its whole record after every move, so late moves take longer:
# the gap between moves grows with the move number.
set(PLY_MS 400)
set(PLY_GROWTH 12)

# A small linear congruential generator, so every platform plays the same game.
set(rng_state ${SEED})
function(lan_random limit out)
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

# Every action of either program, as "time|sequence|program|command"; the
# programs are alpha (A, white) and bravo (B, black).
set(events "")
set(sequence 0)
function(add_event time program command)
  math(EXPR sequence "${sequence} + 1")
  set(sequence ${sequence} PARENT_SCOPE)
  set(time_text "0000000${time}")
  string(LENGTH "${time_text}" length)
  math(EXPR from "${length} - 7")
  string(SUBSTRING "${time_text}" ${from} 7 time_text)
  set(sequence_text "00000${sequence}")
  string(LENGTH "${sequence_text}" length)
  math(EXPR from "${length} - 5")
  string(SUBSTRING "${sequence_text}" ${from} 5 sequence_text)
  list(APPEND events "${time_text}|${sequence_text}|${program}|${command}")
  set(events "${events}" PARENT_SCOPE)
endfunction()

# The lobby: both go to the pre-game chat; bravo invites alpha, who accepts.
add_event(3500 A "click frmWelcome.picChat")
add_event(3500 B "click frmWelcome.picChat")
add_event(11500 B "waitfor frmChat.lstUsers listcount 2")
add_event(11500 B "select frmChat.lstUsers 1")
add_event(11500 B "mousedown frmChat.lstUsers 1 0 5 5")
add_event(11800 B "click frmChat.picInviteAct")
add_event(14500 A "waitfor frmChat.lblInvite(0) caption bravo____")
add_event(14500 A "click frmChat.picTick1")
# Bravo's game form is up once alpha's accept reached it; bravo says so in
# the game chat, and alpha makes its first move only after reading that.
add_event(15000 B "waitfor frmGame.lblColor caption black")
add_event(15000 B "type frmGame.txtMessageOut ready")
add_event(15000 B "click frmGame.picSendMess")
add_event(16000 A "waitfor frmGame.txtMessageIn text ready")

set(game_start 17000)
set(ply_clock ${game_start})
set(turn W)
set(plies 0)
set(previous_message "")
set(winner "")
while(plies LESS ${MAX_PLIES} AND winner STREQUAL "")
  if(turn STREQUAL "W")
    set(opponent B)
    set(mover A)
  else()
    set(opponent W)
    set(mover B)
  endif()
  set(own "")
  set(enemy "")
  foreach(square RANGE 0 63)
    set(piece "${board_${square}}")
    if(NOT piece STREQUAL "")
      string(SUBSTRING "${piece}" 0 1 colour)
      if(colour STREQUAL turn)
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
    lan_random(${own_count} pick)
    list(GET own ${pick} candidate)
    set(piece "${board_${candidate}}")
    string(SUBSTRING "${piece}" 1 1 kind)
    lan_random(100 mode)
    if(mode LESS 12 AND enemy_count GREATER 0)
      lan_random(${enemy_count} pick)
      list(GET enemy ${pick} target)
    else()
      lan_random(64 target)
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
  set(board_${to} "${board_${from}}")
  set(board_${from} "")
  math(EXPR plies "${plies} + 1")
  math(EXPR ply_clock "${ply_clock} + ${PLY_MS} + ${plies} * ${PLY_GROWTH}")
  set(time ${ply_clock})
  # A program moves only after the other's last move has reached it, so a
  # slow machine cannot reorder the game.
  if(NOT previous_message STREQUAL "")
    add_event(
      ${time}
      ${mover}
      "waitfor frmGame.txtGameIn text ${previous_message}"
    )
  endif()
  add_event(${time} ${mover} "mousedown frmGame.picboard(${from}) 1 0 10 10")
  set(step ${time})
  add_event(
    ${step}
    ${mover}
    "drag frmGame.picboard(${from}) frmGame.picboard(${to}) 10 10"
  )
  if(from LESS 10)
    set(from_text "0${from}")
  else()
    set(from_text "${from}")
  endif()
  if(to LESS 10)
    set(to_text "0${to}")
  else()
    set(to_text "${to}")
  endif()
  set(previous_message "${from_text}>${to_text}")
  # A message from the mover every third move.
  math(EXPR chat_turn "${plies} % 3")
  if(chat_turn EQUAL 0)
    lan_random(${chat_count} message_index)
    list(GET chat ${message_index} message)
    math(EXPR step "${time} + 200")
    add_event(${step} ${mover} "type frmGame.txtMessageOut ${message}")
    math(EXPR step "${time} + 250")
    add_event(${step} ${mover} "click frmGame.picSendMess")
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

# The winner calls check mate; both confirm the questions that follow.
if(winner STREQUAL "W")
  set(winner_program A)
  set(loser_program B)
  set(winner_name "alpha____")
else()
  set(winner_program B)
  set(loser_program A)
  set(winner_name "bravo____")
endif()
math(EXPR time "${ply_clock} + 400")
add_event(
  ${time}
  ${loser_program}
  "waitfor frmGame.txtGameIn text ${previous_message}"
)
add_event(${time} ${loser_program} "answer msgbox 6")
math(EXPR time "${time} + 100")
add_event(${time} ${winner_program} "answer msgbox 6")
math(EXPR time "${time} + 100")
add_event(${time} ${winner_program} "click frmGame.picCheckMate")
# The winner waits for the loser's "YouWin!" and the loser for the check mate,
# so neither program ends before the round trip is over.
add_event(${time} ${winner_program} "waitfor frmGame.txtGameIn text YouWin!")
add_event(${time} ${loser_program} "waitfor frmGame.txtGameIn text CheckMate!")
math(EXPR time "${time} + 2500")
add_event(${time} A "#end")
add_event(${time} B "#end")

# The board each program should have saved last, one square per line (CMake
# reads the file with plain line feeds).
set(expected "")
foreach(square RANGE 0 63)
  string(APPEND expected "${board_${square}}\n")
endforeach()

# One working copy and one script per program.
get_filename_component(project_dir "${PROJECT}" DIRECTORY)
get_filename_component(project_file "${PROJECT}" NAME)
list(SORT events)
# The programs start within a few seconds of each other; their clocks begin
# at a shared moment a little later.
string(TIMESTAMP now_seconds "%s" UTC)
math(EXPR epoch_ms "(${now_seconds} + 10) * 1000")
foreach(program A B)
  set(directory "${WORK}-${program}")
  file(REMOVE_RECURSE "${directory}")
  file(COPY "${project_dir}/" DESTINATION "${directory}")
  # The program deletes its saved game when it unloads; keep it for the check.
  file(READ "${directory}/CaptureKing.frm" frm_text)
  string(
    REPLACE "Kill App.Path & \"\\game.tmp\""
    "'Kill game.tmp"
    frm_text
    "${frm_text}"
  )
  file(WRITE "${directory}/CaptureKing.frm" "${frm_text}")
  set(script "wait 3500\n")
  set(clock 0)
  foreach(event IN LISTS events)
    if(NOT event MATCHES "^([0-9]+)\\|([0-9]+)\\|([AB])\\|(.*)$")
      message(FATAL_ERROR "bad event ${event}")
    endif()
    if(NOT CMAKE_MATCH_3 STREQUAL program)
      continue()
    endif()
    # Absolute times: both programs act in step whatever their start-up lag.
    math(EXPR at "${CMAKE_MATCH_1} + 0")
    if(at GREATER clock)
      math(EXPR moment "${epoch_ms} + ${at}")
      string(APPEND script "waitclock ${moment}\n")
      set(clock ${at})
    endif()
    if(NOT CMAKE_MATCH_4 STREQUAL "#end")
      string(APPEND script "${CMAKE_MATCH_4}\n")
    endif()
  endforeach()
  file(WRITE "${WORK}-${program}.ui" "${script}")
endforeach()

execute_process(
  COMMAND
    "${CMAKE_COMMAND}" -E env USERNAME=alpha WFC_LAN_ADDRESS=127.0.0.2
    WFC_LAN_PEERS=127.0.0.2,127.0.0.3 "${WFC}" "${WORK}-A/${project_file}"
    --ui-script "${WORK}-A.ui"
  COMMAND
    "${CMAKE_COMMAND}" -E env USERNAME=bravo WFC_LAN_ADDRESS=127.0.0.3
    WFC_LAN_PEERS=127.0.0.2,127.0.0.3 "${WFC}" "${WORK}-B/${project_file}"
    --ui-script "${WORK}-B.ui"
  RESULTS_VARIABLE results
  ERROR_VARIABLE errors
  TIMEOUT 300
)
foreach(result IN LISTS results)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "a program ended with ${results}: ${errors}")
  endif()
endforeach()
if(NOT "${errors}" STREQUAL "")
  message(FATAL_ERROR "unexpected diagnostics: ${errors}")
endif()

string(LENGTH "${expected}" expected_length)
foreach(program A B)
  file(READ "${WORK}-${program}/game.tmp" saved)
  string(LENGTH "${saved}" saved_length)
  if(saved_length LESS expected_length)
    message(FATAL_ERROR "${program}: game.tmp is too short (${saved_length})")
  endif()
  math(EXPR tail_start "${saved_length} - ${expected_length}")
  string(SUBSTRING "${saved}" ${tail_start} ${expected_length} tail)
  if(NOT tail STREQUAL expected)
    file(WRITE "${WORK}-expected.txt" "${expected}")
    message(
      FATAL_ERROR
      "${program}: the saved board differs: ${WORK}-${program}/game.tmp"
    )
  endif()
endforeach()
message(STATUS "both programs saved the board after ${plies} moves")

# The winner's statistics file now names the winner (the program writes it
# when the loser has confirmed the check mate).
file(READ "${WORK}-${winner_program}/User/User.txt" statistics)
if(NOT statistics MATCHES "^${winner_name}")
  message(FATAL_ERROR "the winner's statistics do not show the win")
endif()
message(STATUS "${winner_name} won; the statistics were updated")

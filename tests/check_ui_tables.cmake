# Fails when src/ui/control_tables.cpp was generated from different evidence
# than the one now in the repository.
# Usage: cmake -DEVIDENCE=<json> -DTABLES=<cpp> -P check_ui_tables.cmake
file(SHA256 "${EVIDENCE}" actual_hash)
file(STRINGS "${TABLES}" hash_lines REGEX "^// SHA256: ")
if(NOT hash_lines)
  message(FATAL_ERROR "${TABLES} has no SHA256 line")
endif()
list(GET hash_lines 0 hash_line)
string(REPLACE "// SHA256: " "" recorded_hash "${hash_line}")
if(NOT recorded_hash STREQUAL actual_hash)
  message(
    FATAL_ERROR
    "control tables are stale: regenerate with scripts/New-UiControlTables.ps1"
  )
endif()

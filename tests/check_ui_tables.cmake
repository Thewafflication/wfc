# Fails when src/ui/control_tables.cpp was generated from different evidence
# than the one now in the repository. Line endings are normalized first so a
# checkout that converts them still matches.
# Usage: cmake -DEVIDENCE=<json> -DTABLES=<cpp> -P check_ui_tables.cmake
file(READ "${EVIDENCE}" evidence_text)
string(REPLACE "\r\n" "\n" evidence_text "${evidence_text}")
string(SHA256 actual_hash "${evidence_text}")
file(STRINGS "${TABLES}" hash_lines REGEX "^// SHA256: ")
if(NOT hash_lines)
  message(FATAL_ERROR "${TABLES} has no SHA256 line")
endif()
list(GET hash_lines 0 hash_line)
string(REPLACE "// SHA256: " "" recorded_hash "${hash_line}")
string(STRIP "${recorded_hash}" recorded_hash)
if(NOT recorded_hash STREQUAL actual_hash)
  message(
    FATAL_ERROR
    "control tables are stale: regenerate with scripts/New-UiControlTables.ps1"
  )
endif()

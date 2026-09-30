# REQ-0253 — `Collection` default member and `ReDim` of UDT arrays

## Statement

- `c(index)` / `c("key")` read a `Collection`'s default `Item` member.
- `ReDim` / `ReDim Preserve` of a dynamic array of a user-defined type gives
  every new slot its own instance (closes the gap noted in `REQ-0241`).

## Scope

The default-member shortcut is limited to the built-in `Collection`; general
`Attribute ... VB_UserMemId = 0` default members in user classes are not
implemented (the project loader blanks `Attribute` lines).

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-collection-default-cli`.

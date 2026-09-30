# REQ-0237 — `Enum`

## Statement

`[Public|Private] Enum Name` ... `End Enum` declares Long module constants.
Members default to the previous value + 1 (first is 0); `Member = constant
expression` sets it explicitly. `Name` is accepted as a type wherever
`Long` is (`Dim`, parameters, return types); values are plain Longs.

## Scope

Not covered: range checking against the Enum, `[_Hidden]` members,
qualified `Name.Member` access, Enums inside classes.

## Verification

`tests/evaluator_tests.cpp` Enum case; `TC-MP0002-enum-cli`.

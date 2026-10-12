# REQ-0266 — `Declare` statements

## Statement

`[Public|Private] Declare [PtrSafe] Function|Sub Name Lib "dll" [Alias "x"]
[(params)] [As Type]` is accepted so programs that mention Win32 APIs load.
The name is registered as an external routine. A few routines are emulated
natively (`winapi.cpp`); on Windows every other declared routine is called in its
DLL (`REQ-0307`). A routine that cannot be called raises run-time error 453
("Specified DLL function not found"), catchable with `On Error`.

## Scope

Calling real DLLs is `REQ-0307`. Before MP-0008 declared routines were never
executed.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-declare-cli`.

# REQ-0266 — `Declare` statements

## Statement

`[Public|Private] Declare [PtrSafe] Function|Sub Name Lib "dll" [Alias "x"]
[(params)] [As Type]` is accepted so programs that mention Win32 APIs load.
The name is registered as an external routine: calling it raises run-time
error 453 ("Specified DLL function not found"), catchable with `On Error`.

## Scope

No foreign-function interface: declared routines are never executed, and the
parameter list is not validated. A real DLL-call bridge is out of scope for
MP-0002.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-declare-cli`.

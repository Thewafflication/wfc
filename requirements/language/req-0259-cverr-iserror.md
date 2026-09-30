# REQ-0259 — `CVErr` and `IsError`

## Statement

`CVErr(n)` (0–65535) returns an error-subtype Variant; `IsError(v)` is True
only for one. `TypeName` is `Error`, `VarType` is 10, `CStr`/`Print`/`&`
render `Error n`, and two error values compare equal when their numbers are
equal. This replaces the former constant-`False` `IsError` and closes the
`CVErr` item the earlier backlog had scoped out.

## Scope

Arithmetic and numeric conversion of an error value report a type mismatch
rather than propagating the error value (real VB6 propagates it through
arithmetic), and `CLng(CVErr(n))` does not return `n`.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-cverr-cli`.

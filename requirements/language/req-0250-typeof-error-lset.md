# REQ-0250 — `TypeOf ... Is`, `Error n`, `LSet`/`RSet`, `Erl`, `Command`

## Statement

- `TypeOf obj Is ClassName` (or `Is Object`): True when the object's class
  is, or implements, `ClassName`; `Nothing` is False.
- `Error n` raises run-time error `n` like `Err.Raise n`.
- `LSet`/`RSet` copy a String into the variable's current width, left- or
  right-justified, padding with spaces or truncating.
- `Erl` is always 0 (line numbers are not tracked); `Command`/`Command$`
  are always empty.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-typeof-error-lset-cli`.

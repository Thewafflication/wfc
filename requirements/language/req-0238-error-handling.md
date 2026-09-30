# REQ-0238 — Error handling and `GoTo`

## Statement

- `On Error Resume Next`, `On Error GoTo label`, `On Error GoTo 0`;
- `Resume`, `Resume Next`, `Resume label`;
- `GoTo label` and `label:` statements (procedure-local; also usable in a
  single-line `If ... Then GoTo label`, `Exit`, `Err.Raise`);
- `Err.Number`, `Err.Description`, `Err.Source` (always empty),
  `Err.Clear`, `Err.Raise number[, source[, description]]`.

Catchable runtime errors: division by zero (11), overflow (6), subscript out
of range (9), object not set (91), invalid use of Null (94), invalid
procedure call (5), stack depth (28), and `Err.Raise`d numbers. An error in
a callee with no handler propagates to the caller's handler. An error
raised while a `GoTo` handler is active (before `Resume`/`On Error`) is not
re-handled.

## Scope

- Block statements (`If`/`For`/`While`/`Do`/`Select`/`With`) whose own
  header expression fails are not recovered; their nested statements are.
- `Erl`, `Err` as a bare object, `Resume` semantics for `Err.Raise` across
  frames beyond the above, and `On Error` inside `Class_Terminate` are not
  covered. Syntax errors are never catchable.

## Verification

`tests/evaluator_tests.cpp` error-handling cases; `TC-MP0002-error-handling-cli`.

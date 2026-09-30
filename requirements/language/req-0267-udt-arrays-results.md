# REQ-0267 — UDT arrays as parameters, UDT function results, Variant copies

## Statement

- `Sub F(a() As Pt)` accepts an array of a user-defined type (or any class)
  by reference; the argument's element class must match.
- A `Function` declared `As Pt` returns a value-semantics UDT: its result
  slot starts as a fresh instance and `F = r` copies.
- Storing a UDT into a `Variant` (assignment, `Variant` parameter, Variant
  field or array element) copies it, and the built-in `Collection.Add` takes
  its item ByVal, so later changes to the original do not leak in.

## Scope

Returning a UDT array, and passing a UDT array to a `Variant` parameter,
are not handled specially.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-udt-arrays-results-cli`.

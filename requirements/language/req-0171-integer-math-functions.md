# REQ-0171 — Integer math function subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140, REQ-0160, and REQ-0077

## Requirement

`Abs(<number>)` and `Sgn(<number>)` shall each require one `Long` argument and
return a `Long`.

`Abs` returns the non-negative magnitude. Applying it to -2147483648 shall fail
with the existing integer-overflow diagnostic because its positive magnitude is
not representable as a signed 32-bit value.

`Sgn` returns -1 for a negative argument, 0 for zero, and 1 for a positive
argument. Calls may be nested in other expressions and intrinsic calls.

Variant preservation, Decimal/Currency, and floating-point arguments remain
outside this requirement.

## Verification

- `tests/evaluator_tests.cpp` covers negative, zero, positive, nested, type
  mismatch, and overflow behavior.
- `TC-MP0002-integer-math-cli` verifies both functions through `wfc --eval`.

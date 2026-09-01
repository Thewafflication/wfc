# REQ-0183 — Floating-point math functions

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0077 and REQ-0181

## Requirement

The evaluator shall recognize the case-insensitive one-argument functions
`Sqr`, `Sin`, `Cos`, `Tan`, `Atn`, `Exp`, and `Log`. Each accepts a `Long` or
`Double` argument (widened to `Double`) and returns a `Double`:

- `Sqr` returns the square root and rejects a negative argument with `WFC0096`;
- `Log` returns the natural logarithm and rejects an argument that is not
  positive with `WFC0096`;
- `Sin`, `Cos`, and `Tan` interpret the argument as radians;
- `Atn` returns the arctangent in radians;
- `Exp` returns e raised to the argument.

A non-numeric argument fails with `WFC0073` and wrong arity fails with
`WFC0072`. A result outside the finite `Double` range fails with `WFC0009`
instead of introducing an infinity into the value model.

## Scope

These are the intrinsic transcendental functions of the VBA `Math` module
(`REQ-0077`) that the `Double` type (`REQ-0181`) makes representable. `Rnd` and
`Randomize` (which return `Single` and carry generator state) and hyperbolic or
base-10 helpers remain deferred.

## Verification

- `tests/evaluator_tests.cpp` covers exact results (`Sqr(9)`, `Exp(0)`,
  `Log(1)`, `Sin(0)`, `Cos(0)`, `Tan(0)`), the `Atn` identity for pi, an
  `Exp`/`Log` round trip, domain rejection for `Sqr`/`Log`, and a non-numeric
  argument. It also verifies `Exp` overflow rejection.
- `TC-MP0002-float-math-cli` verifies representative functions through
  `wfc --eval`.

## Traceability

This requirement partially implements `REQ-0077` and consumes `REQ-0181`.

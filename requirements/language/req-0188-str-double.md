# REQ-0188 — Str over Long and Double

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0072 and REQ-0181

## Requirement

`Str(<number>)` and `Str$(<number>)` shall accept a `Long` or `Double` and
return its deterministic decimal representation. A nonnegative value shall
have one leading space reserved for its sign; a negative value shall begin
with `-`. Positive and negative numeric zero shall render as ` 0`.

A non-numeric argument shall report `WFC0073`; wrong arity shall report
`WFC0072`.

## Scope

`Double` digits use the evaluator's shortest round-tripping invariant decimal
form. Locale-specific decimal separators and the complete VB6 display-format
heuristics remain deferred to `Format` and the locale layer. `Single`,
`Currency`, and `Decimal` inputs remain deferred with those value types.

## Verification

- `tests/evaluator_tests.cpp` covers positive, negative, zero, negative zero,
  exponent input, both function spellings, and non-numeric rejection.
- `TC-MP0002-str-cli` verifies positive and negative fractional values through
  `wfc --eval`.

## Traceability

This requirement extends the `Str` portion of `REQ-0072` to the `Double` model
introduced by `REQ-0181`.

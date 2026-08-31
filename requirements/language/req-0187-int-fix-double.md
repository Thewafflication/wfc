# REQ-0187 — Int and Fix over Long and Double

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0077 and REQ-0181

## Requirement

`Int(<number>)` and `Fix(<number>)` shall accept a `Long` or `Double` and
preserve its numeric type. A `Long` is already integral and remains unchanged.
For `Double`, `Int` rounds downward toward negative infinity while `Fix`
truncates toward zero. Consequently, both agree for non-negative values but
`Int(-2.5)` is `-3` while `Fix(-2.5)` is `-2`.

A non-numeric argument shall report `WFC0073`; wrong arity shall report
`WFC0072`.

## Scope

`Single`, `Currency`, and `Decimal` preservation remains deferred with those
distinct value types.

## Verification

- `tests/evaluator_tests.cpp` covers positive and negative `Double` values,
  unchanged `Long` values, result types, and non-numeric rejection.
- `TC-MP0002-int-fix-cli` verifies the negative fractional distinction through
  `wfc --eval`.

## Traceability

This requirement implements the `Int` and `Fix` portion of `REQ-0077` for the
numeric model introduced by `REQ-0181`.

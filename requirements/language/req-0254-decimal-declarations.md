# REQ-0254 — `Decimal` as a declared type

## Statement

`Decimal` is accepted as a type in `Dim`/`Static`/`Const`/parameters/
returns/fields/arrays (previously only `CDec` and `Variant` could hold one).
Integer, Byte, Currency, Single, and Double values convert implicitly on
assignment (`Double` through the nearest-representable `Decimal`, as
`CDec` does); arithmetic, rendering, and `TypeName` reuse the existing
`Decimal` implementation (`REQ-0198`).

## Scope

Assigning a `Decimal` to a narrower fixed type (`Long`, `Double`) still
needs an explicit conversion function.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-decimal-type-cli`.

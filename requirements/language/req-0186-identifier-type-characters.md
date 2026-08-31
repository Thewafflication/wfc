# REQ-0186 — Representable identifier type-declaration characters

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0142, REQ-0153, and REQ-0184

## Requirement

The evaluator shall recognize the identifier type-declaration characters that
map exactly to its current scalar value types:

- `#` declares or asserts `Double`;
- `&` declares or asserts `Long`;
- `$` declares or asserts `String`.

`Dim name<character>` and `Const name<character> = expression` shall infer the
declared type without an `As` clause. The type character is not part of the
identifier's name: suffixed and unsuffixed references identify the same symbol,
and duplicate declarations are rejected accordingly. A suffix on a reference
shall assert the symbol's type and report `WFC0016` on mismatch. Combining a
declaration character with an `As` clause shall report `WFC0012`.

The existing `$` intrinsic-function aliases remain available when the suffixed
identifier is followed by an argument list.

## Scope

The `!`, `%`, and `@` characters report `WFC0097` until distinct `Single`,
`Integer`, and `Currency` value types exist. This requirement does not add
implicit `Variant` declarations.

## Verification

- `tests/evaluator_tests.cpp` covers inferred variables and constants,
  suffixed/unsuffixed aliasing, explicit `As Double` references through `#`,
  duplicate declarations, mismatched assertions, redundant `As`, and an
  unsupported `!` declaration.
- `TC-MP0002-identifier-type-characters-cli` verifies inferred `Double` and
  `String` declarations through `wfc --eval`.

## Traceability

This requirement extends the declaration grammar from `REQ-0142` and
`REQ-0153` to the representable legacy characters and consumes the `Double`
declaration support in `REQ-0184`.

# REQ-0185 — Representable numeric literal suffixes

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0140 and REQ-0181

## Requirement

The evaluator shall recognize the VB numeric literal type-declaration
characters that map exactly to its current numeric value types:

- `#` forces a literal to `Double`, including an otherwise integral literal;
- `&` confirms an integer literal as `Long`.

The suffix is part of the literal token. A fractional or exponent-form literal
with `&` shall report `WFC0006`. A `#` literal shall use `Double` range rather
than first being constrained to the `Long` range. Whitespace-delimited `&`
shall remain the concatenation operator.

## Scope

This requirement does not yet implement identifier type-declaration
characters. The `!`, `%`, and `@` literal suffixes remain deferred until the
evaluator has distinct `Single`, `Integer`, and `Currency` value types; mapping
them to `Double` or `Long` would produce incorrect `TypeName` and `VarType`
semantics.

## Verification

- `tests/evaluator_tests.cpp` covers forced `Double`, explicit `Long`, signed
  values, a `Double` value above the `Long` range, introspection, invalid
  fractional/exponent `Long` forms, and preservation of concatenation.
- `TC-MP0002-numeric-literal-suffixes-cli` verifies both supported suffixes
  through `wfc --eval`.

## Traceability

This requirement extends the numeric literal grammar in `REQ-0140` with the
two declaration characters representable by `REQ-0181`.

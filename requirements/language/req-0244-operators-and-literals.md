# REQ-0244 — `^`, bitwise logic, `Like`, `&H`/`&O` literals, precedence, string `+`

## Statement

- `^` (Double result, left-associative, tighter than unary minus: `-2 ^ 2` is
  `-4`).
- `And`/`Or`/`Xor`/`Eqv`/`Imp`/`Not` on `Integer`/`Long` operands are
  bitwise (a `Boolean` mixed with an integer counts as `-1`/`0`).
- `Like` with `?`, `*`, `#`, `[list]`, `[!list]`, ranges; honours
  `Option Compare Text`.
- `&HFF`, `&O17`, optional `&` Long suffix; unsuffixed values up to
  `&HFFFF` are `Integer` (two's-complement wrap), larger are `Long`.
- Binary precedence now follows VB: `* /` > `\` > `Mod` > `+ -` > `&`.
- `String + String` concatenates; `&` accepts Boolean operands.

## Scope

`Like` character classes such as `[[]` escapes, `Decimal ^`, and `String + Number`
coercion (`"1" + 2`) are not covered.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-operators-cli`. Three older
negative tests (string `+`, integer `And`, Boolean `&`) were updated because
those forms are now valid.

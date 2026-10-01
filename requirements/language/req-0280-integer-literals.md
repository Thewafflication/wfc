# REQ-0280 — Unsuffixed small integer literals are `Integer`

## Statement

- An unsuffixed decimal integer literal in -32768 through 32767 has type
  `Integer`; one beyond that range up to the `Long` limits is a `Long`, and
  one beyond `Long` is a `Double` (REQ-0279 addendum). `&`, `%`, `#`, `!`, `@`
  suffixes behave as before.
- Consequences match VB6: `TypeName(5)` is "Integer"; `5 + 5` is an `Integer`;
  `32767 + 1` and `200 * 200` raise error 6 (Overflow); `1000& * 1000` is a `Long`.
- Whole-number values of any width (`Integer`, `Byte`, `Long`) are accepted
  wherever a Long is needed: `Spc`/`Tab` counts, `String * n` lengths, `Open ... Len =`,
  library arguments, `Sleep`.

## Verification

Updated expectations in `tests/evaluator_tests.cpp` and the `typename`, `vartype`,
`cvar`, `variant`, `const-inferred-type` and variant array CLI tests; all corpus programs.

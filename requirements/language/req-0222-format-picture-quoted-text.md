# REQ-0222 — Format custom picture quoted literal text

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0218, REQ-0220, REQ-0221

## Requirement

The evaluator shall recognize a pair of `"` characters inside a
`Format`/`Format$` custom numeric picture (`REQ-0218`) as delimiting a
run of literal text: every character between them is rendered exactly
as written, regardless of whether it would otherwise be interpreted as
one of this picture format's own special characters, and the `"`
delimiters themselves never appear in the output.

- Real VB6 offers two ways to force a literal character in a custom
  picture — `\` (`REQ-0221`, one character at a time) and `"..."` (this
  requirement, a whole run at once) — and they compose freely: a picture
  may use either, both, or neither.
- A `;` inside a `"..."` run is not treated as a `REQ-0220` section
  separator, the same as an escaped `\;` — checked ahead of section
  splitting, since a section boundary must be decided before any one
  section's own quoted runs are otherwise expanded.
- A quoted run left unterminated (no closing `"` before the picture
  section ends) makes every remaining character in that section literal
  too — a disclosed simplification, since real VB6 pictures do not
  normally end mid-quote.
- Since a `Style` argument is itself a VB6 string literal, embedding a
  literal `"` inside one already requires VB6's own doubled-quote
  escaping (`""`) at the language level, before this requirement's own
  picture-level `"..."` delimiting ever sees the resulting string value
  — for example `Format(1234, "0 ""units""")` is VB6 source for the
  four-character `Style` value `0 "units"`, which this requirement then
  renders as `1234 units`.

## Diagnostics

None specific to this requirement — a quoted run is never rejected,
only rendered (or, if left unterminated, treated as running to the end
of its section).

## Scope

This requirement adds `"..."` quoted literal text. It does not add:

- `%`/`E+`/`E-` combined with a custom picture — unchanged from
  `REQ-0218`'s own exclusion;
- a fourth (text) custom-picture section, or the `Date`/`Time` named
  styles — unchanged from `REQ-0193`'s own exclusions;
- an escape sequence *inside* a quoted run (for example whether `\"`
  ends the run early or is a literal backslash followed by the closing
  quote) — this evaluator's quoted runs do not give `\` any special
  meaning at all while inside one, a disclosed simplification not
  independently verified against the reference runtime, since VB6's own
  documentation does not describe `\`/`"..."` interacting this way.

## Verification

- `tests/evaluator_tests.cpp` covers quoted literal text after a digit
  placeholder, quoted literal text before one, a quoted `;` not
  splitting a picture into sections (mirroring `REQ-0221`'s equivalent
  `\;` test), and quoted literal text combined with a real, unquoted
  section separator selecting between a positive and negative rendering.
- `TC-MP0002-format-quoted-text-cli` verifies quoted text after and
  before a placeholder, and the combined-with-a-real-separator case,
  through `wfc --eval`.

## Reference

- [Microsoft VBA `Format` function reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/format-function)

## Traceability

This requirement extends `REQ-0218`'s custom numeric picture with VB6's
second literal-text mechanism, alongside `REQ-0221`'s `\` escape
character, and makes `REQ-0220`'s `;` section-splitting aware of it the
same way it is already aware of `\`.

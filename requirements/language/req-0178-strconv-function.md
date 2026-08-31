# REQ-0178 — StrConv case-conversion subset

## Requirement

The MP-0002 evaluator shall recognize the case-insensitive `StrConv(String,
Conversion)` intrinsic with exactly two arguments, where `Conversion` is a
`Long` selected from the `VbStrConv` values (`REQ-0084`):

- `vbUpperCase` (1) shall return the String with every ASCII letter uppercased;
- `vbLowerCase` (2) shall return the String with every ASCII letter lowercased;
- `vbProperCase` (3) shall uppercase the first ASCII letter of each maximal run
  of letters and lowercase the remaining letters, leaving non-letters
  unchanged.

Wrong arity shall fail with `WFC0072`, a non-`Long` conversion argument shall
fail with `WFC0073`, and any other conversion value — including `vbWide`,
`vbNarrow`, `vbKatakana`, `vbHiragana`, `vbUnicode`, and `vbFromUnicode` — shall
fail with `WFC0093`.

The `VbStrConv` constants shall be source-visible reserved names resolving to
their `REQ-0084` values, so that source referencing an unsupported conversion
compiles and fails only at execution.

## Scope

The current evaluator stores Strings as ASCII-compatible byte sequences. Only
the case-mapping conversions are representable; the width, kana, and
Unicode/DBCS conversions require locale and multi-byte semantics the evaluator
does not yet model and are therefore rejected at execution rather than silently
approximated.

## Verification

Unit tests cover uppercase, lowercase, and proper-case conversion (including
digits within words), wrong arity, wrong conversion type, and rejection of an
unsupported conversion. `TC-MP0002-strconv-cli` covers proper-case conversion
through `wfc --eval`.

## Traceability

This requirement partially implements `REQ-0071` and consumes the `VbStrConv`
constant contract `REQ-0084`.

# REQ-0220 — Format multi-section custom numeric pictures

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0218

## Requirement

The evaluator shall recognize a `Format`/`Format$` custom numeric picture
(`REQ-0218`) containing one or two `;` separators as up to three
semicolon-separated sections — `positive[;negative[;zero]]` — and render
the widened `Expression` through whichever section real VB6 selects for
it, rather than always applying the whole `Style` string as a single
picture.

- One section (no `;` at all) applies to every value, unchanged from
  `REQ-0218`.
- Two sections: the first applies to a positive value or zero, the second
  to a negative value.
- Three sections: the first applies to a positive value, the second to a
  negative value, and the third to zero (checked ahead of the
  positive/negative choice, so it takes priority for a zero value even
  though `0.0` is not negative).
- A negative value rendered through the dedicated negative section (the
  second section, whenever more than one section is present) uses its own
  magnitude (`Abs`), not the signed value: the negative section gets no
  automatic leading `-` the way the single-section form does, so any sign
  shown must come from the section's own literal characters (for example
  `Format(-1234.5, "0.00;(0.00)")` is `"(1234.50)"`, with the parentheses
  supplying the negative indication, not a `-`).
- A fourth section (VB6's text section, used only when `Expression` is a
  `String`) is out of scope: `REQ-0193`'s Scope already excludes applying
  any named or custom `Format` style to a `String` expression at all.

Manual testing while implementing this requirement surfaced two real bugs
in `REQ-0218`'s existing single-section rendering, both now fixed (see
that requirement's updated Requirement/Scope sections) because the
canonical use case for a *negative* section is a parenthesized or
currency-prefixed picture (`"(0.00)"`, `"$#,##0;($#,##0)"`), which
triggered both:

- extra integer digits beyond the picture's placeholder count were
  inserted ahead of the *entire* picture, including a literal character
  that sits to the left of the leftmost placeholder (`Format(1234,
  "(0)")` rendered `"1234(0)"`/`"123(4)"` instead of `"(1234)"`);
- comma grouping was computed by counting every character in the
  rendered integer section from its end, rather than only its actual
  digit positions, so a literal character trailing the last placeholder
  (a closing `)`) miscounted the grouping and produced digits grouped in
  the wrong places (`Format(1234567, "$#,##0;($#,##0)")` rendered
  `"$1,234,567"` correctly but `Format(-1234567, "$#,##0;($#,##0)")`
  rendered a garbled `"(,$12,345,67)"` instead of `"($1,234,567)"`).

## Diagnostics

None specific to this requirement — a multi-section picture, like a
single-section one, is never rejected, only rendered.

## Scope

This requirement adds up to three `;`-separated numeric sections. It does
not add:

- a fourth (text) section, for a `String` `Expression` — unchanged from
  `REQ-0193`'s existing exclusion of `String` from every `Format`
  style, named or custom;
- an *empty* section (`";;"`, or a trailing `;` with nothing after it)
  independently verified against the reference runtime — an empty
  section has no digit placeholders and no literal characters, so it
  renders as an empty string via the same "no placeholders" rule
  `REQ-0218` already establishes, but this exact shape was not
  separately probed;
- `%`/`E+`/`E-` combined with a custom picture in any section, or a `\`
  escape character — unchanged from `REQ-0218`'s own exclusions, which
  apply equally to every section.

## Verification

- `tests/evaluator_tests.cpp` covers two-section positive/negative
  selection (with and without a value of zero going to the shared first
  section), three-section zero-section selection, a negative section
  combined with `$`-prefixed comma grouping, and the two `REQ-0218`
  regressions this requirement's testing found (a leading literal with
  overflow digits, and comma grouping with a trailing literal).
- `TC-MP0002-format-sections-cli` verifies two-section parenthesized
  negative rendering, three-section currency-style grouping (both signs),
  and three-section zero selection, through `wfc --eval`.

## Reference

- [Microsoft VBA `Format` function reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/format-function)

## Traceability

This requirement extends `REQ-0218`'s single-section custom numeric
picture with the multi-section form its own Scope section listed as
deferred, and fixes two bugs in that requirement's own rendering logic
that this requirement's own manual testing uncovered.

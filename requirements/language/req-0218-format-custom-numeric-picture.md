# REQ-0218 — Format custom numeric picture strings

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0193

## Requirement

The evaluator shall recognize any `Format`/`Format$` `Style` argument that
does not name one of `REQ-0193`'s reserved named styles as a VBA custom
numeric picture, rendering it character by character against the widened
`Expression` (`Long`, `Double`, or `Boolean`, per `REQ-0193`).

- `0` is a digit placeholder: shows a digit if one remains at that
  position, otherwise forces `0`.
- `#` is a digit placeholder: shows a digit if one remains at that
  position, otherwise shows nothing.
- `.` marks the single decimal point, splitting the picture into an
  integer section (before it) and a fraction section (after it). The
  value is rounded to the fraction section's placeholder count using the
  same nearest-even rounding `Fixed`/`Standard`/`Round` already use. A
  trailing run of fraction digits that rounded to `0` and came from a `#`
  placeholder is trimmed (for example `Format(1.5, "0.0#")` is `"1.5"`,
  not `"1.50"`). A picture with no `.` has no fraction section or decimal
  point in its output at all.
- `,` appearing among the integer section's digit placeholders enables
  comma-grouped thousands separators for the whole integer section (for
  example `"#,##0.00"`).
- Every other character is a literal, copied through at its position in
  the output, interleaved with the digit placeholders.
- The integer section is matched against the value's digits from the
  rightmost placeholder leftward; if the value has more digits than the
  picture has placeholders, the extra leading digits are still shown in
  full ahead of the picture's own leftmost character — a picture's digit
  placeholders set a *minimum* width, never a maximum.
- A negative value gets a leading `-` unless the entire rounded magnitude
  (every digit shown) is `0`, matching `Fixed`/`Standard`'s own
  convention.
- An empty `Style` string behaves exactly like the one-argument form of
  `Format` (real VB6 behavior, not previously covered).

## Diagnostics

None specific to this requirement — a custom picture is never rejected,
only rendered. This retires `WFC0102` ("Format does not yet support this
Style value"), which this requirement's fallback used to report
unconditionally for any unrecognized `Style`; every other `Format`
diagnostic (`WFC0072`/`WFC0073`/`WFC0009`) is unchanged.

## Scope

This requirement adds the four core picture tokens (`0`, `#`, `.`, `,`)
and literal passthrough. It does not add:

- `%` or `E+`/`E-` inside a custom picture (scaling/scientific notation
  combined with a custom digit pattern) — use the existing named
  `Percent`/`Scientific` styles instead;
- multiple `;`-separated sections (`positive;negative;zero;text`) — every
  picture is treated as a single section, applied to every value the same
  way regardless of sign;
- a `\` escape character for a literal digit-placeholder/comma/period
  character;
- string picture tokens (`@`, `!`, `<`, `>`) — unchanged from `REQ-0193`'s
  own exclusion;
- applying a picture to a `String` expression — unchanged from
  `REQ-0193`'s own exclusion;
- verified fidelity for a picture that mixes literal characters *into*
  the same run as a grouping comma (for example a hyphen or space
  breaking up a digit-placeholder-and-comma sequence) or a picture with
  no digit placeholders at all in its integer section (the value's digits
  are still rendered, prepended ahead of the entire literal text) — both
  are a reasoned, disclosed interpretation of the "never truncate the
  integer part" rule above, not independently verified against the
  reference runtime.

## Verification

- `tests/evaluator_tests.cpp` covers zero-padding (`"0000"`), a forced
  two-decimal picture, grouped thousands with rounding
  (`"#,##0.00"`), a negative zero-padded value, trailing optional-zero
  trimming (`"0.0#"`), negative-sign suppression at an all-zero result,
  a picture with more placeholders than digits (`"###0"`), an empty
  `Style`, literal text before and after digit placeholders, `Boolean`
  widening, a decimal-only picture (`".00"`), integer-only grouping
  (`"#,##0"`), and rounding to zero fraction digits (`"0"`).
- `TC-MP0002-format-cli` verifies zero-padding and grouped-thousands
  custom pictures through `wfc --eval`.

## Traceability

This requirement extends `REQ-0193`'s named-style `Format` foundation
with the custom numeric picture form its own Scope section listed as
deferred, retiring the `WFC0102` diagnostic that deferral used to report.

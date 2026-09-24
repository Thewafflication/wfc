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
  comma-grouped thousands separators, counted over the integer section's
  actual rendered *digit* positions only — a literal character before,
  within, or after that digit run (for example the `$`/`(` in
  `"$#,##0"`/`"($#,##0)"`) is never itself counted toward a group of
  three, and never shifts where a separator lands.
- Every other character is a literal, copied through at its position in
  the output, interleaved with the digit placeholders.
- The integer section is matched against the value's digits from the
  rightmost placeholder leftward; if the value has more digits than the
  picture has placeholders, the extra leading digits are still shown in
  full immediately next to the leftmost placeholder's own digit — a
  picture's digit placeholders set a *minimum* width, never a maximum —
  without being pushed past a literal character that sits further left
  in the picture still (for example `Format(1234, "(0)")` is `"(1234)"`,
  not `"1234(0)"` or `"123(4)"`). A picture with *no* digit placeholder at
  all in its integer section has no "leftmost placeholder" for the extra
  digits to sit next to, so they are shown ahead of the entire literal
  text instead (unchanged from this requirement's original behavior, for
  example `Format(42, "hello")` is `"42hello"`).
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
- multiple `;`-separated sections (`positive;negative;zero;text`) —
  originally excluded here (every picture was treated as a single
  section, applied to every value the same way regardless of sign);
  `REQ-0220` later added the three numeric sections (a fourth, text,
  section stays excluded, alongside the existing "no `String` expression"
  exclusion below);
- a `\` escape character for a literal digit-placeholder/comma/period
  character;
- string picture tokens (`@`, `!`, `<`, `>`) — unchanged from `REQ-0193`'s
  own exclusion;
- applying a picture to a `String` expression — unchanged from
  `REQ-0193`'s own exclusion;
- verified fidelity for a picture with no digit placeholders at all in
  its integer section (the value's digits are still rendered, prepended
  ahead of the entire literal text) — a reasoned, disclosed
  interpretation of the "never truncate the integer part" rule above, not
  independently verified against the reference runtime. (A literal
  character that sits *before*, interleaved among, or *after* the integer
  section's actual digit placeholders — a leading `$`/`(`, a trailing
  `)`, or similar — was originally the same kind of unverified disclosure
  here too; manual testing during `REQ-0220`'s own work found it produced
  outright wrong output, not just unverified output, for the two most
  common real-world shapes (a leading literal combined with overflow
  digits, and comma grouping combined with a trailing literal), so both
  were fixed and are now covered by tests instead of disclosed as
  unverified.)

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

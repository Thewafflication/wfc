# REQ-0221 — Format custom picture `\` escape character

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0218, REQ-0220

## Requirement

The evaluator shall recognize a `\` inside a `Format`/`Format$` custom
numeric picture (`REQ-0218`) as an escape character: the character
immediately following it is always rendered as a plain literal at its
position, even when it would otherwise be interpreted as one of this
picture format's own special characters.

- `\0` and `\#` are literal `0`/`#` characters, never digit placeholders.
- `\,` is a literal comma, never a grouping-enable instruction — an
  escaped comma does not itself enable comma grouping, and does not
  count as one of the section's placeholders.
- `\.` is a literal decimal point, never the integer/fraction section
  separator — a picture whose only `.` is escaped this way has no
  fraction section at all, the same as a picture with no `.` in it.
- `\;` is a literal semicolon, never a `REQ-0220` section separator — a
  picture containing an escaped `;` is not split at that position, even
  though `REQ-0220`'s own section-splitting logic runs first.
- `\\` is a literal `\`: the first `\` escapes the second, which is then
  rendered as an ordinary literal character (not re-interpreted as the
  start of a further escape).
- A `\` with no following character (the very last character of a
  picture section) is a disclosed simplification: rather than being
  rejected or silently dropped, it is shown as a literal `\` itself,
  since there is nothing left for it to escape.
- Every other character is unaffected by this requirement: an
  already-literal character (a letter, digit-outside-a-placeholder-
  context, space, `$`, `(`, `)`, etc.) may be escaped too, with no
  visible difference from leaving it unescaped.

## Diagnostics

None specific to this requirement — an escape sequence is never
rejected, only rendered.

## Scope

This requirement adds the `\` escape character for the four picture
tokens above (`0`, `#`, `,`, `.`) and the `;` section separator. It does
not add:

- a way to escape one of the *named* `Format` styles (`Fixed`,
  `Currency`, ...) into a literal custom picture — a `Style` string is
  only ever a custom picture once it fails to name a reserved style
  (`REQ-0193`), unchanged;
- quote-delimited literal text (`"..."`), VB6's other mechanism for
  embedding characters that would otherwise be interpreted specially —
  originally excluded here (every character inside a pair of `"` in a
  custom picture was still interpreted the same as it would be outside
  one); `REQ-0222` later added it.

## Verification

- `tests/evaluator_tests.cpp` covers an escaped `0` (three of them, with
  no unescaped placeholder at all in the picture), an escaped comma, an
  escaped semicolon (both alone and combined with a real `REQ-0220`
  section separator later in the same picture, verifying both the
  positive and negative section selection still work), an escaped
  decimal point, and an escaped backslash (`\\` rendering as a literal
  `\`).
- `TC-MP0002-format-escape-cli` verifies the escaped-`0`, escaped-comma,
  escaped-semicolon, escaped-decimal-point, and escaped-backslash cases
  through `wfc --eval`.

## Reference

- [Microsoft VBA `Format` function reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/format-function)

## Traceability

This requirement extends `REQ-0218`'s custom numeric picture (and
`REQ-0220`'s multi-section splitting, which it also makes escape-aware
for `;`) with the `\` escape character both requirements' Scope sections
listed as deferred.

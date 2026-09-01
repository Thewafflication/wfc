# REQ-0175 — CByte function subset

## Requirement

The MP-0002 evaluator shall recognize the case-insensitive `CByte` intrinsic
with exactly one numeric argument. A `Double` argument shall be rounded to the
nearest even integer, and the result shall use the current `Long`
representation when it is in the inclusive range 0 through 255.

Values outside that range shall fail with integer-overflow diagnostic
`WFC0009`. Non-numeric current value types shall fail with type diagnostic `WFC0073`.

## Scope

This increment deliberately excludes Currency, Date, Variant, and locale-aware
String conversion. Those forms require value types and conversion semantics
that MP-0002 does not yet represent.

## Verification

Unit and CLI tests cover both endpoints, an interior value, nearest-even
`Double` rounding, post-rounding range overflow, and rejection of a String argument.

## Traceability

This requirement partially implements `REQ-0072`.

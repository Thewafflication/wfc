# REQ-0175 — CByte function subset

## Requirement

The MP-0002 evaluator shall recognize the case-insensitive `CByte` intrinsic
with exactly one `Long` argument and shall return that value as the current
`Long` representation when it is in the inclusive range 0 through 255.

Values outside that range shall fail with integer-overflow diagnostic
`WFC0009`. Other current value types shall fail with type diagnostic `WFC0073`.

## Scope

This increment deliberately excludes floating-point, Currency, Date, Variant,
and locale-aware String conversion. Those forms require value types and
rounding semantics that MP-0002 does not yet represent.

## Verification

Unit and CLI tests cover both endpoints, an interior value, range overflow,
and rejection of a String argument.

## Traceability

This requirement partially implements `REQ-0072`.

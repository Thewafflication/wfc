# REQ-0175 — CByte function subset

## Requirement

The MP-0002 evaluator shall recognize the case-insensitive `CByte` intrinsic
with exactly one numeric argument or one `String` containing a complete finite
decimal/exponent number after surrounding ASCII whitespace is removed. A
`Double` argument or numeric `String` shall be rounded to the nearest even
integer, and the result shall use the current `Long` representation when it is
in the inclusive range 0 through 255. A `Boolean` argument shall convert to
255 for `True` and zero for `False`, matching VBA Byte coercion.

Values outside that range shall fail with integer-overflow diagnostic
`WFC0009`. A malformed numeric `String` shall fail with `WFC0098`; other
non-numeric current value types shall fail with type diagnostic `WFC0073`.

## Scope

This increment deliberately excludes Currency, Date, Variant, and
locale-sensitive number formats. Those forms require value types and conversion
semantics that MP-0002 does not yet represent.

## Verification

Unit and CLI tests cover both endpoints, an interior value, nearest-even
`Double` and numeric-`String` rounding, Boolean conversion, surrounding
whitespace, exponent notation, post-rounding range overflow, and
malformed-String rejection. Unit tests also cover both wrong-arity forms.

## Traceability

This requirement partially implements `REQ-0072`.

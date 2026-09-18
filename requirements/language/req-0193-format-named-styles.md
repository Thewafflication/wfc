# REQ-0193 — Format named numeric styles

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0071, REQ-0072, REQ-0160, REQ-0172, and REQ-0181

## Requirement

The MP-0002 evaluator shall recognize case-insensitive `Format` and `Format$`
with one or two arguments: `Format(<expression>)` and
`Format(<expression>, <style>)`.

With one argument, `Format` shall return the same String as the current
`CStr`/`Str` rendering of the argument's value: base-10 digits for a `Long`,
the shortest round-tripping decimal form for a `Double`, `True`/`False` for a
`Boolean`, and the input unchanged for a `String`.

With two arguments, the `Expression` shall be a `Long`, `Double`, or `Boolean`
and `Style` shall be a `String` naming one of the following case-insensitive
named formats. A `Boolean` widens to -1 (`True`) or 0 (`False`) before a
numeric style applies.

| Style | Result |
| --- | --- |
| `General Number` | Digits with no forced decimals, thousands separator, or leading space: a `Long` renders exactly, a `Double` uses the shortest round-tripping form, a `Boolean` widens to `-1`/`0`. |
| `Fixed` | The widened value with exactly two decimal digits, a leading `-` only when the rounded magnitude is nonzero, and no thousands separator. |
| `Standard` | `Fixed`, with a comma every three integer digits. |
| `Percent` | The widened value multiplied by 100, rendered like `Fixed`, with a trailing `%`. |
| `Scientific` | One mantissa digit, two fraction digits, an uppercase `E`, an explicit exponent sign, and a minimum two-digit exponent (for example `1.23E+03`). |
| `Yes/No` | `Yes` for a nonzero widened value, otherwise `No`. |
| `True/False` | `True` for a nonzero widened value, otherwise `False`. |
| `On/Off` | `On` for a nonzero widened value, otherwise `Off`. |

`Fixed`, `Standard`, and `Percent` round to two decimal digits using the same
nearest-even rounding as `Round`, `Hex`, and `Oct`.

## Diagnostics

`WFC0072` reports wrong arity. `WFC0073` reports a non-String `Style`, or an
`Expression` that is not a `Long`, `Double`, or `Boolean` when `Style` is
given. `WFC0009` reports a `Percent` scaling result outside the finite
`Double` range. `WFC0102` reports a `Style` String that does not name one of
the eight styles above.

## Scope

This increment implements only the eight named styles listed above, which
require no locale table and no distinct `Currency`, `Date`, or `Single` type.
It does not implement:

- custom numeric picture strings (`0`, `#`, `,`, `.`, `%`, `E+`/`E-`, and
  literal/quoted text);
- the `Currency`, `General Date`, `Long Date`, `Medium Date`, `Short Date`,
  `Long Time`, `Medium Time`, and `Short Time` named styles, which need a
  `Currency` or `Date` value type and a locale policy that do not yet exist;
- string picture tokens (`@`, `!`, `<`, `>`) and the `FirstDayOfWeek`/
  `FirstWeekOfYear` parameters from the type-library signature in `REQ-0071`;
  and
- applying a named style to a `String` expression, which VBA coerces through
  its general numeric/date parsing rules that this evaluator does not yet
  implement for `Format`.

A `Style` naming one of these deferred forms reports `WFC0102` rather than a
false result.

## Verification

- `tests/evaluator_tests.cpp` covers the one-argument form for every current
  value type, all eight named styles (including negative, zero, large,
  fractional, and grouped-thousands inputs), Boolean widening, both wrong-arity
  forms, non-String `Style`, non-numeric `Expression` with a `Style`, `Percent`
  overflow, and an unrecognized `Style` name.
- `TC-MP0002-format-cli` verifies representative styles through `wfc --eval`.

## Reference

- [Microsoft VBA `Format` function reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/format-function)

## Traceability

This requirement partially implements the `_B_str_Format`/`_B_var_Format`
members of `REQ-0071`.

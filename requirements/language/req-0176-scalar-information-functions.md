# REQ-0176 — Scalar Information function subset

## Requirement

The MP-0002 evaluator shall implement the VBA Information functions whose
behavior can be represented by its current `Long`, `Double`, `Boolean`, and `String`
value model:

- `IsNumeric`, `TypeName`, and `VarType` shall classify current scalar values.
  `IsNumeric` shall recognize complete, finite decimal and exponent strings
  after trimming ASCII whitespace;
- `IsArray`, `IsObject`, `IsNull`, `IsEmpty`, `IsError`, and `IsMissing` shall
  return `False`, because none of those value categories can currently exist;
- `QBColor(index)` shall map indices 0 through 15 to the corresponding VBA
  palette color value and reject indices outside that range;
- `RGB(red, green, blue)` shall require three `Long` components, reject negative
  components, clamp components above 255, and return
  `red + green * 256 + blue * 65536`.

Calls with the wrong arity shall fail with `WFC0072`. Calls whose arguments do
not meet the function's current scalar contract shall fail with `WFC0073`.
Negative `RGB` components shall fail with `WFC0091`.
Out-of-range `QBColor` indices shall fail with `WFC0092`.

## Scope

This increment does not represent arrays, objects, `Null`, `Empty`, error
values, omitted Variant arguments, dates, or runtime VBA type information.
`IsDate` and true results for the unavailable value categories are deferred
until their supporting value models exist.

## Verification

Unit tests cover scalar classification, all constant-False predicates, RGB
component ordering, clamping, arity, type rejection, negative components, and
the complete sixteen-entry `QBColor` palette contract.
`TC-MP0002-isnumeric-cli`, `TC-MP0002-typename-cli`,
`TC-MP0002-vartype-cli`, `TC-MP0002-info-predicates-cli`, and
`TC-MP0002-rgb-cli`, and `TC-MP0002-qbcolor-cli` cover the implemented
functions through `wfc --eval`.

## Traceability

This requirement partially implements `REQ-0075`.

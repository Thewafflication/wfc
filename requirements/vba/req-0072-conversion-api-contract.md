# REQ-0072 — VBA Conversion module API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic for Applications 6.0 type library (`VBA6.DLL`)

## Scope

This requirement applies to the VBA `Conversion` standard-library module.

## Requirement

The WFC VBA runtime shall expose every function and property listed below with the specified type-library name, member kind, DISPID, return type, parameters, optionality, and default values.

## Member Inventory

| Member | Kind | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | --- | ---: | --- |
| `_B_str_Hex` | Method | `Number: Variant` | `String` | 1610612736 | — |
| `_B_var_Hex` | Method | `Number: Variant` | `Variant` | 1610612737 | — |
| `_B_str_Oct` | Method | `Number: Variant` | `String` | 1610612738 | — |
| `_B_var_Oct` | Method | `Number: Variant` | `Variant` | 1610612739 | — |
| `MacID` | Method | `Constant: String` | `Long` | 1610612740 | Hidden |
| `_B_str_Str` | Method | `Number: Variant` | `String` | 1610612741 | — |
| `_B_var_Str` | Method | `Number: Variant` | `Variant` | 1610612742 | — |
| `Val` | Method | `String: String` | `Double` | 1610612743 | — |
| `CStr` | Method | `Expression: Variant` | `String` | 1610612744 | — |
| `CByte` | Method | `Expression: Variant` | `Byte` | 1610612745 | — |
| `CBool` | Method | `Expression: Variant` | `Boolean` | 1610612746 | — |
| `CCur` | Method | `Expression: Variant` | `Currency` | 1610612747 | — |
| `CDate` | Method | `Expression: Variant` | `Date` | 1610612748 | — |
| `CVDate` | Method | `Expression: Variant` | `Variant` | 1610612749 | — |
| `CInt` | Method | `Expression: Variant` | `Integer` | 1610612750 | — |
| `CLng` | Method | `Expression: Variant` | `Long` | 1610612751 | — |
| `CSng` | Method | `Expression: Variant` | `Single` | 1610612752 | — |
| `CDbl` | Method | `Expression: Variant` | `Double` | 1610612753 | — |
| `CVar` | Method | `Expression: Variant` | `Variant` | 1610612754 | — |
| `CVErr` | Method | `Expression: Variant` | `Variant` | 1610612755 | — |
| `_B_str_Error` | Method | `ErrorNumber: Variant optional` | `String` | 1610612756 | — |
| `_B_var_Error` | Method | `ErrorNumber: Variant optional` | `Variant` | 1610612757 | — |
| `Fix` | Method | `Number: Variant` | `Variant` | 1610612758 | — |
| `Int` | Method | `Number: Variant` | `Variant` | 1610612759 | — |
| `CDec` | Method | `Expression: Variant` | `Variant` | 1610612760 | — |

## Rationale

This contract is part of the source-visible VBA standard library required
to compile and execute compatible VB and VBA code.

## Verification

**Method:** Inspection

**References:** `evidence/reference/vba-6.0-type-library.json`; test case TBD

Inspect the WFC VBA type information and compare every listed name, value,
member identifier, access mode, and signature. Function behavior requires
separate executable reference and conformance tests.

## Relationships

- **Derived from:** MP-0001 compatibility discovery objective
- **Depends on:** VBA value, string, date, file, and host interaction architecture
- **Conflicts with:** None known

## Tailoring

A member or value may be deferred only through an approved compatibility-scope
change that identifies the resulting source or run-time limitation.

## Implementation Record

The source evaluator implements `Hex`, `Oct`, `Str`, decimal/radix `Val`,
`MacID`, `CStr`, `CByte`, `CBool`, `CInt`, `CLng`, `CSng`, `CDbl`, `CCur`,
`CDec`, current-scalar `CVar`, `Fix`, `Int`, and a bounded `Error`/`Error$`
catalog through REQ-0170, REQ-0172 through REQ-0175, REQ-0181, REQ-0182,
REQ-0187 through REQ-0192, and REQ-0195 through REQ-0199. `CSng` returns a
genuine `Single` value (`REQ-0195`), not a `Double`-narrowed-to-float-
precision value. `CCur` (`REQ-0196`) rounds to the nearest exact `Currency`
tick rather than a binary-floating-point approximation. `CDec` (`REQ-0198`)
returns an exact 96-bit-mantissa `Decimal`, reachable only through a scalar
`Variant` (`REQ-0197`). `CInt` (`REQ-0199`) returns a genuine 16-bit
`Integer` value, not a `Long`-narrowed-to-Integer-range value. `CDate`,
`CVDate`, `CVErr`, complete Variant behavior (arrays, objects, and error
values remain out of scope per `REQ-0197`), and runtime type information
beyond `TypeName`/`VarType` remain pending.

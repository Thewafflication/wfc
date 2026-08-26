# REQ-0075 — VBA Information module API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic for Applications 6.0 type library (`VBA6.DLL`)

## Scope

This requirement applies to the VBA `Information` standard-library module.

## Requirement

The WFC VBA runtime shall expose every function and property listed below with the specified type-library name, member kind, DISPID, return type, parameters, optionality, and default values.

## Member Inventory

| Member | Kind | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | --- | ---: | --- |
| `Erl` | Method | `` | `Long` | 1610612736 | Hidden |
| `Err` | Method | `` | `ErrObject` | 1610612737 | — |
| `IMEStatus` | Method | `` | `VbIMEStatus` | 1610612738 | — |
| `IsArray` | Method | `VarName: Variant` | `Boolean` | 1610612739 | — |
| `IsDate` | Method | `Expression: Variant` | `Boolean` | 1610612740 | — |
| `IsEmpty` | Method | `Expression: Variant` | `Boolean` | 1610612741 | — |
| `IsError` | Method | `Expression: Variant` | `Boolean` | 1610612742 | — |
| `IsMissing` | Method | `ArgName: Variant` | `Boolean` | 1610612743 | — |
| `IsNull` | Method | `Expression: Variant` | `Boolean` | 1610612744 | — |
| `IsNumeric` | Method | `Expression: Variant` | `Boolean` | 1610612745 | — |
| `IsObject` | Method | `Expression: Variant` | `Boolean` | 1610612746 | — |
| `TypeName` | Method | `VarName: Variant` | `String` | 1610612747 | — |
| `VarType` | Method | `VarName: Variant` | `VbVarType` | 1610612748 | — |
| `QBColor` | Method | `Color: Integer` | `Long` | 1610612749 | — |
| `RGB` | Method | `Red: Integer, Green: Integer, Blue: Integer` | `Long` | 1610612750 | — |

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

Not implemented.

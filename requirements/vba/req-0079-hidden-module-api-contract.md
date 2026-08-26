# REQ-0079 — VBA _HiddenModule module API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic for Applications 6.0 type library (`VBA6.DLL`)

## Scope

This requirement applies to the VBA `_HiddenModule` standard-library module.

## Requirement

The WFC VBA runtime shall expose every function and property listed below with the specified type-library name, member kind, DISPID, return type, parameters, optionality, and default values.

## Member Inventory

| Member | Kind | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | --- | ---: | --- |
| `Array` | Method | `ArgList: Variant() optional` | `Variant` | 1610612736 | — |
| `_B_str_InputB` | Method | `Number: Long, FileNumber: Integer` | `String` | 1610612737 | — |
| `_B_var_InputB` | Method | `Number: Long, FileNumber: Integer` | `Variant` | 1610612738 | — |
| `_B_str_Input` | Method | `Number: Long, FileNumber: Integer` | `String` | 1610612739 | — |
| `_B_var_Input` | Method | `Number: Long, FileNumber: Integer` | `Variant` | 1610612740 | — |
| `Width` | Method | `FileNumber: Integer, Width: Integer` | `Void` | 1610612741 | — |
| `VarPtr` | Method | `Ptr: Void` | `Long` | 1610612742 | Hidden |
| `StrPtr` | Method | `Ptr: String` | `Long` | 1610612743 | Hidden |
| `ObjPtr` | Method | `Ptr: IUnknown` | `Long` | 1610612744 | Hidden |

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

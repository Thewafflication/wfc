# REQ-0073 — VBA FileSystem module API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic for Applications 6.0 type library (`VBA6.DLL`)

## Scope

This requirement applies to the VBA `FileSystem` standard-library module.

## Requirement

The WFC VBA runtime shall expose every function and property listed below with the specified type-library name, member kind, DISPID, return type, parameters, optionality, and default values.

## Member Inventory

| Member | Kind | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | --- | ---: | --- |
| `ChDir` | Method | `Path: String` | `Void` | 1610612736 | — |
| `ChDrive` | Method | `Drive: String` | `Void` | 1610612737 | — |
| `EOF` | Method | `FileNumber: Integer` | `Boolean` | 1610612738 | — |
| `FileAttr` | Method | `FileNumber: Integer, ReturnType: Integer = 1` | `Long` | 1610612739 | — |
| `FileCopy` | Method | `Source: String, Destination: String` | `Void` | 1610612740 | — |
| `FileDateTime` | Method | `PathName: String` | `Variant` | 1610612741 | — |
| `FileLen` | Method | `PathName: String` | `Long` | 1610612742 | — |
| `GetAttr` | Method | `PathName: String` | `VbFileAttribute` | 1610612743 | — |
| `Kill` | Method | `PathName: Variant` | `Void` | 1610612744 | — |
| `Loc` | Method | `FileNumber: Integer` | `Long` | 1610612745 | — |
| `LOF` | Method | `FileNumber: Integer` | `Long` | 1610612746 | — |
| `MkDir` | Method | `Path: String` | `Void` | 1610612747 | — |
| `Reset` | Method | `` | `Void` | 1610612748 | — |
| `RmDir` | Method | `Path: String` | `Void` | 1610612749 | — |
| `Seek` | Method | `FileNumber: Integer` | `Long` | 1610612750 | — |
| `SetAttr` | Method | `PathName: String, Attributes: VbFileAttribute` | `Void` | 1610612751 | — |
| `_B_str_CurDir` | Method | `Drive: Variant optional` | `String` | 1610612752 | — |
| `_B_var_CurDir` | Method | `Drive: Variant optional` | `Variant` | 1610612753 | — |
| `FreeFile` | Method | `RangeNumber: Variant optional` | `Integer` | 1610612754 | — |
| `Dir` | Method | `PathName: Variant optional, Attributes: VbFileAttribute = 0` | `String` | 1610612755 | — |

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

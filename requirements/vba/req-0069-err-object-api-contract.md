# REQ-0069 — VBA ErrObject class API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic for Applications 6.0 type library (`VBA6.DLL`)

## Scope

This requirement applies to the VBA `ErrObject` run-time class.

## Requirement

The WFC VBA runtime shall expose the `ErrObject` properties and methods listed below with the specified names, DISPIDs, access modes, and signatures.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Reference CLSID | `{A4C46654-499F-101B-BB78-00AA00383CBB}` |
| Default interface | `_ErrObject`, `{A4C466B8-499F-101B-BB78-00AA00383CBB}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `Description` | Get/Let | `String` | 1610743812 | — |
| `HelpContext` | Get/Let | `Long` | 1610743816 | — |
| `HelpFile` | Get/Let | `String` | 1610743814 | — |
| `LastDllError` | Get | `Long` | 1610743820 | — |
| `Number` | Get/Let | `Long` | 0 | — |
| `Source` | Get/Let | `String` | 1610743810 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `Raise` | `Number: Long, Source: Variant optional, Description: Variant optional, HelpFile: Variant optional, HelpContext: Variant optional` | `Void` | 1610743818 | — |
| `Clear` | `` | `Void` | 1610743819 | — |

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
- **Depends on:** VBA object and error model architecture
- **Conflicts with:** None known

## Tailoring

A member or value may be deferred only through an approved compatibility-scope
change that identifies the resulting source or run-time limitation.

## Implementation Record

Not implemented.

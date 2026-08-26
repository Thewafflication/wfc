# REQ-0070 — VBA Collection class API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic for Applications 6.0 type library (`VBA6.DLL`)

## Scope

This requirement applies to the VBA `Collection` run-time class.

## Requirement

The WFC VBA runtime shall expose the `Collection` properties and methods listed below with the specified names, DISPIDs, access modes, and signatures.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Reference CLSID | `{A4C4671C-499F-101B-BB78-00AA00383CBB}` |
| Default interface | `_Collection`, `{A4C46780-499F-101B-BB78-00AA00383CBB}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `Item` | `Index: Variant` | `Variant` | 0 | — |
| `Add` | `Item: Variant, Key: Variant optional, Before: Variant optional, After: Variant optional` | `Void` | 1 | — |
| `Count` | `` | `Long` | 2 | — |
| `Remove` | `Index: Variant` | `Void` | 3 | — |
| `_NewEnum` | `` | `IUnknown` | -4 | — |

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

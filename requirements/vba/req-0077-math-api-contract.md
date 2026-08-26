# REQ-0077 — VBA Math module API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic for Applications 6.0 type library (`VBA6.DLL`)

## Scope

This requirement applies to the VBA `Math` standard-library module.

## Requirement

The WFC VBA runtime shall expose every function and property listed below with the specified type-library name, member kind, DISPID, return type, parameters, optionality, and default values.

## Member Inventory

| Member | Kind | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | --- | ---: | --- |
| `Abs` | Method | `Number: Variant` | `Variant` | 1610612736 | — |
| `Atn` | Method | `Number: Double` | `Double` | 1610612737 | — |
| `Cos` | Method | `Number: Double` | `Double` | 1610612738 | — |
| `Exp` | Method | `Number: Double` | `Double` | 1610612739 | — |
| `Log` | Method | `Number: Double` | `Double` | 1610612740 | — |
| `Randomize` | Method | `Number: Variant optional` | `Void` | 1610612741 | — |
| `Rnd` | Method | `Number: Variant optional` | `Single` | 1610612742 | — |
| `Sin` | Method | `Number: Double` | `Double` | 1610612743 | — |
| `Sqr` | Method | `Number: Double` | `Double` | 1610612744 | — |
| `Tan` | Method | `Number: Double` | `Double` | 1610612745 | — |
| `Sgn` | Method | `Number: Variant` | `Variant` | 1610612746 | — |
| `Round` | Method | `Number: Variant, NumDigitsAfterDecimal: Long = 0` | `Variant` | 1610612747 | — |

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

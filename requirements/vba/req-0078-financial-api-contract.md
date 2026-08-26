# REQ-0078 — VBA Financial module API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic for Applications 6.0 type library (`VBA6.DLL`)

## Scope

This requirement applies to the VBA `Financial` standard-library module.

## Requirement

The WFC VBA runtime shall expose every function and property listed below with the specified type-library name, member kind, DISPID, return type, parameters, optionality, and default values.

## Member Inventory

| Member | Kind | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | --- | ---: | --- |
| `SLN` | Method | `Cost: Double, Salvage: Double, Life: Double` | `Double` | 1610612736 | — |
| `SYD` | Method | `Cost: Double, Salvage: Double, Life: Double, Period: Double` | `Double` | 1610612737 | — |
| `DDB` | Method | `Cost: Double, Salvage: Double, Life: Double, Period: Double, Factor: Variant optional` | `Double` | 1610612738 | — |
| `IPmt` | Method | `Rate: Double, Per: Double, NPer: Double, PV: Double, FV: Variant optional, Due: Variant optional` | `Double` | 1610612739 | — |
| `PPmt` | Method | `Rate: Double, Per: Double, NPer: Double, PV: Double, FV: Variant optional, Due: Variant optional` | `Double` | 1610612740 | — |
| `Pmt` | Method | `Rate: Double, NPer: Double, PV: Double, FV: Variant optional, Due: Variant optional` | `Double` | 1610612741 | — |
| `PV` | Method | `Rate: Double, NPer: Double, Pmt: Double, FV: Variant optional, Due: Variant optional` | `Double` | 1610612742 | — |
| `FV` | Method | `Rate: Double, NPer: Double, Pmt: Double, PV: Variant optional, Due: Variant optional` | `Double` | 1610612743 | — |
| `NPer` | Method | `Rate: Double, Pmt: Double, PV: Double, FV: Variant optional, Due: Variant optional` | `Double` | 1610612744 | — |
| `Rate` | Method | `NPer: Double, Pmt: Double, PV: Double, FV: Variant optional, Due: Variant optional, Guess: Variant optional` | `Double` | 1610612745 | — |
| `IRR` | Method | `ValueArray: Double(), Guess: Variant optional` | `Double` | 1610612746 | — |
| `MIRR` | Method | `ValueArray: Double(), FinanceRate: Double, ReinvestRate: Double` | `Double` | 1610612747 | — |
| `NPV` | Method | `Rate: Double, ValueArray: Double()` | `Double` | 1610612748 | — |

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

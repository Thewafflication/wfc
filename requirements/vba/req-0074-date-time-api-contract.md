# REQ-0074 — VBA DateTime module API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic for Applications 6.0 type library (`VBA6.DLL`)

## Scope

This requirement applies to the VBA `DateTime` standard-library module.

## Requirement

The WFC VBA runtime shall expose every function and property listed below with the specified type-library name, member kind, DISPID, return type, parameters, optionality, and default values.

## Member Inventory

| Member | Kind | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | --- | ---: | --- |
| `_B_var_Date` | PropertyGet | `` | `Variant` | 1610612736 | — |
| `_B_str_Date` | PropertyLet | `` | `String` | 1610612737 | — |
| `_B_var_Date` | PropertyLet | `` | `Variant` | 1610612736 | — |
| `_B_str_Date` | PropertyGet | `` | `String` | 1610612737 | — |
| `DateSerial` | Method | `Year: Integer, Month: Integer, Day: Integer` | `Variant` | 1610612740 | — |
| `DateValue` | Method | `Date: String` | `Variant` | 1610612741 | — |
| `Day` | Method | `Date: Variant` | `Variant` | 1610612742 | — |
| `Hour` | Method | `Time: Variant` | `Variant` | 1610612743 | — |
| `Minute` | Method | `Time: Variant` | `Variant` | 1610612744 | — |
| `Month` | Method | `Date: Variant` | `Variant` | 1610612745 | — |
| `Now` | PropertyGet | `` | `Variant` | 1610612746 | — |
| `Second` | Method | `Time: Variant` | `Variant` | 1610612747 | — |
| `_B_var_Time` | PropertyGet | `` | `Variant` | 1610612748 | — |
| `_B_str_Time` | PropertyLet | `` | `String` | 1610612749 | — |
| `_B_var_Time` | PropertyLet | `` | `Variant` | 1610612748 | — |
| `_B_str_Time` | PropertyGet | `` | `String` | 1610612749 | — |
| `Timer` | PropertyGet | `` | `Single` | 1610612752 | — |
| `TimeSerial` | Method | `Hour: Integer, Minute: Integer, Second: Integer` | `Variant` | 1610612753 | — |
| `TimeValue` | Method | `Time: String` | `Variant` | 1610612754 | — |
| `Weekday` | Method | `Date: Variant, FirstDayOfWeek: VbDayOfWeek = 1` | `Variant` | 1610612755 | — |
| `Year` | Method | `Date: Variant` | `Variant` | 1610612756 | — |
| `DateAdd` | Method | `Interval: String, Number: Double, Date: Variant` | `Variant` | 1610612757 | — |
| `DateDiff` | Method | `Interval: String, Date1: Variant, Date2: Variant, FirstDayOfWeek: VbDayOfWeek = 1, FirstWeekOfYear: VbFirstWeekOfYear = 1` | `Variant` | 1610612758 | — |
| `DatePart` | Method | `Interval: String, Date: Variant, FirstDayOfWeek: VbDayOfWeek = 1, FirstWeekOfYear: VbFirstWeekOfYear = 1` | `Variant` | 1610612759 | — |
| `Calendar` | PropertyGet | `` | `VbCalendar` | 1610612760 | — |
| `Calendar` | PropertyLet | `` | `VbCalendar` | 1610612760 | — |

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

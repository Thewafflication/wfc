# REQ-0071 — VBA Strings module API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic for Applications 6.0 type library (`VBA6.DLL`)

## Scope

This requirement applies to the VBA `Strings` standard-library module.

## Requirement

The WFC VBA runtime shall expose every function and property listed below with the specified type-library name, member kind, DISPID, return type, parameters, optionality, and default values.

## Member Inventory

| Member | Kind | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | --- | ---: | --- |
| `Asc` | Method | `String: String` | `Integer` | 1610612736 | — |
| `_B_str_Chr` | Method | `CharCode: Long` | `String` | 1610612737 | — |
| `_B_var_Chr` | Method | `CharCode: Long` | `Variant` | 1610612738 | — |
| `_B_str_LCase` | Method | `String: String` | `String` | 1610612739 | — |
| `_B_var_LCase` | Method | `String: Variant` | `Variant` | 1610612740 | — |
| `_B_str_Mid` | Method | `String: String, Start: Long, Length: Variant optional` | `String` | 1610612741 | — |
| `_B_var_Mid` | Method | `String: Variant, Start: Long, Length: Variant optional` | `Variant` | 1610612742 | — |
| `_B_str_MidB` | Method | `String: String, Start: Long, Length: Variant optional` | `String` | 1610612743 | — |
| `_B_var_MidB` | Method | `String: Variant, Start: Long, Length: Variant optional` | `Variant` | 1610612744 | — |
| `InStr` | Method | `Start: Variant optional, String1: Variant optional, String2: Variant optional, Compare: VbCompareMethod = 0` | `Variant` | 1610612745 | — |
| `InStrB` | Method | `Start: Variant optional, String1: Variant optional, String2: Variant optional, Compare: VbCompareMethod = 0` | `Variant` | 1610612746 | — |
| `_B_str_Left` | Method | `String: String, Length: Long` | `String` | 1610612747 | — |
| `_B_var_Left` | Method | `String: Variant, Length: Long` | `Variant` | 1610612748 | — |
| `_B_str_LeftB` | Method | `String: String, Length: Long` | `String` | 1610612749 | — |
| `_B_var_LeftB` | Method | `String: Variant, Length: Long` | `Variant` | 1610612750 | — |
| `_B_str_LTrim` | Method | `String: String` | `String` | 1610612751 | — |
| `_B_var_LTrim` | Method | `String: Variant` | `Variant` | 1610612752 | — |
| `_B_str_RightB` | Method | `String: String, Length: Long` | `String` | 1610612753 | — |
| `_B_var_RightB` | Method | `String: Variant, Length: Long` | `Variant` | 1610612754 | — |
| `_B_str_Right` | Method | `String: String, Length: Long` | `String` | 1610612755 | — |
| `_B_var_Right` | Method | `String: Variant, Length: Long` | `Variant` | 1610612756 | — |
| `_B_str_RTrim` | Method | `String: String` | `String` | 1610612757 | — |
| `_B_var_RTrim` | Method | `String: Variant` | `Variant` | 1610612758 | — |
| `_B_str_Space` | Method | `Number: Long` | `String` | 1610612759 | — |
| `_B_var_Space` | Method | `Number: Long` | `Variant` | 1610612760 | — |
| `_B_var_StrConv` | Method | `String: Variant, Conversion: VbStrConv, LocaleID: Long = 0` | `Variant` | 1610612761 | — |
| `_B_str_String` | Method | `Number: Long, Character: Variant` | `String` | 1610612762 | — |
| `_B_var_String` | Method | `Number: Long, Character: Variant` | `Variant` | 1610612763 | — |
| `_B_str_Trim` | Method | `String: String` | `String` | 1610612764 | — |
| `_B_var_Trim` | Method | `String: Variant` | `Variant` | 1610612765 | — |
| `_B_str_UCase` | Method | `String: String` | `String` | 1610612766 | — |
| `_B_var_UCase` | Method | `String: Variant` | `Variant` | 1610612767 | — |
| `StrComp` | Method | `String1: Variant, String2: Variant, Compare: VbCompareMethod = 0` | `Variant` | 1610612768 | — |
| `_B_str_Format` | Method | `Expression: Variant, Format: Variant optional, FirstDayOfWeek: VbDayOfWeek = 1, FirstWeekOfYear: VbFirstWeekOfYear = 1` | `String` | 1610612769 | — |
| `_B_var_Format` | Method | `Expression: Variant, Format: Variant optional, FirstDayOfWeek: VbDayOfWeek = 1, FirstWeekOfYear: VbFirstWeekOfYear = 1` | `Variant` | 1610612770 | — |
| `Len` | Method | `Expression: Variant` | `Variant` | 1610612771 | — |
| `LenB` | Method | `Expression: Variant` | `Variant` | 1610612772 | — |
| `AscB` | Method | `String: String` | `Byte` | 1610612773 | — |
| `_B_str_ChrB` | Method | `CharCode: Byte` | `String` | 1610612774 | — |
| `_B_var_ChrB` | Method | `CharCode: Byte` | `Variant` | 1610612775 | — |
| `AscW` | Method | `String: String` | `Integer` | 1610612776 | — |
| `_B_str_ChrW` | Method | `CharCode: Long` | `String` | 1610612777 | — |
| `_B_var_ChrW` | Method | `CharCode: Long` | `Variant` | 1610612778 | — |
| `FormatDateTime` | Method | `Expression: Variant, NamedFormat: VbDateTimeFormat = 0` | `String` | 1610612779 | — |
| `FormatNumber` | Method | `Expression: Variant, NumDigitsAfterDecimal: Integer = -1, IncludeLeadingDigit: VbTriState = -2, UseParensForNegativeNumbers: VbTriState = -2, GroupDigits: VbTriState = -2` | `String` | 1610612780 | — |
| `FormatPercent` | Method | `Expression: Variant, NumDigitsAfterDecimal: Integer = -1, IncludeLeadingDigit: VbTriState = -2, UseParensForNegativeNumbers: VbTriState = -2, GroupDigits: VbTriState = -2` | `String` | 1610612781 | — |
| `FormatCurrency` | Method | `Expression: Variant, NumDigitsAfterDecimal: Integer = -1, IncludeLeadingDigit: VbTriState = -2, UseParensForNegativeNumbers: VbTriState = -2, GroupDigits: VbTriState = -2` | `String` | 1610612782 | — |
| `WeekdayName` | Method | `Weekday: Integer, Abbreviate: Boolean = False, FirstDayOfWeek: VbDayOfWeek = 0` | `String` | 1610612783 | — |
| `MonthName` | Method | `Month: Integer, Abbreviate: Boolean = False` | `String` | 1610612784 | — |
| `Replace` | Method | `Expression: String, Find: String, Replace: String, Start: Long = 1, Count: Long = -1, Compare: VbCompareMethod = 0` | `String` | 1610612785 | — |
| `StrReverse` | Method | `Expression: String` | `String` | 1610612786 | — |
| `Join` | Method | `SourceArray: Variant, Delimiter: Variant optional` | `String` | 1610612787 | — |
| `Filter` | Method | `SourceArray: Variant, Match: String, Include: Boolean = True, Compare: VbCompareMethod = 0` | `Variant` | 1610612788 | — |
| `InStrRev` | Method | `StringCheck: String, StringMatch: String, Start: Long = -1, Compare: VbCompareMethod = 0` | `Long` | 1610612789 | — |
| `Split` | Method | `Expression: String, Delimiter: Variant optional, Limit: Long = -1, Compare: VbCompareMethod = 0` | `Variant` | 1610612790 | — |

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

The source evaluator implements the current scalar String subset through
`REQ-0160`--`REQ-0169`, `REQ-0177`, and `REQ-0178`, including the byte-string
family over its stored byte representation and the `StrConv` case-mapping
conversions. DBCS code-page semantics, width/kana/Unicode `StrConv`
conversions, Variant/array members, formatting functions, runtime type
information, and remaining module members are pending.

# REQ-0076 — VBA Interaction module API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic for Applications 6.0 type library (`VBA6.DLL`)

## Scope

This requirement applies to the VBA `Interaction` standard-library module.

## Requirement

The WFC VBA runtime shall expose every function and property listed below with the specified type-library name, member kind, DISPID, return type, parameters, optionality, and default values.

## Member Inventory

| Member | Kind | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | --- | ---: | --- |
| `AppActivate` | Method | `Title: Variant, Wait: Variant optional` | `Void` | 1610612736 | — |
| `Beep` | Method | `` | `Void` | 1610612737 | — |
| `CreateObject` | Method | `Class: String, ServerName: String = ""` | `Variant` | 1610612738 | — |
| `DoEvents` | Method | `` | `Integer` | 1610612739 | — |
| `GetObject` | Method | `PathName: Variant optional, Class: Variant optional` | `Variant` | 1610612740 | — |
| `InputBox` | Method | `Prompt: Variant, Title: Variant optional, Default: Variant optional, XPos: Variant optional, YPos: Variant optional, HelpFile: Variant optional, Context: Variant optional` | `String` | 1610612741 | — |
| `MacScript` | Method | `Script: String` | `String` | 1610612742 | Hidden |
| `MsgBox` | Method | `Prompt: Variant, Buttons: VbMsgBoxStyle = 0, Title: Variant optional, HelpFile: Variant optional, Context: Variant optional` | `VbMsgBoxResult` | 1610612743 | — |
| `SendKeys` | Method | `String: String, Wait: Variant optional` | `Void` | 1610612744 | — |
| `Shell` | Method | `PathName: Variant, WindowStyle: VbAppWinStyle = 2` | `Double` | 1610612745 | — |
| `Partition` | Method | `Number: Variant, Start: Variant, Stop: Variant, Interval: Variant` | `Variant` | 1610612746 | — |
| `Choose` | Method | `Index: Single, Choice: Variant() optional` | `Variant` | 1610612747 | — |
| `_B_var_Environ` | Method | `Expression: Variant` | `Variant` | 1610612748 | — |
| `_B_str_Environ` | Method | `Expression: Variant` | `String` | 1610612749 | — |
| `Switch` | Method | `VarExpr: Variant() optional` | `Variant` | 1610612750 | — |
| `_B_var_Command` | Method | `` | `Variant` | 1610612751 | — |
| `_B_str_Command` | Method | `` | `String` | 1610612752 | — |
| `IIf` | Method | `Expression: Variant, TruePart: Variant, FalsePart: Variant` | `Variant` | 1610612753 | — |
| `GetSetting` | Method | `AppName: String, Section: String, Key: String, Default: Variant optional` | `String` | 1610612754 | — |
| `SaveSetting` | Method | `AppName: String, Section: String, Key: String, Setting: String` | `Void` | 1610612755 | — |
| `DeleteSetting` | Method | `AppName: String, Section: Variant optional, Key: Variant optional` | `Void` | 1610612756 | — |
| `GetAllSettings` | Method | `AppName: String, Section: String` | `Variant` | 1610612757 | — |
| `CallByName` | Method | `Object: Object, ProcName: String, CallType: VbCallType, Args: Variant() optional` | `Variant` | 1610612758 | — |

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

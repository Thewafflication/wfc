# Visual Basic for Applications 6.0 Requirements

**Content type:** Project requirement index

**Status:** Proposed

**Reference baseline:** Installed `VBA6.DLL` 6.0.8169, VBA type library 6.0

This set captures the two VBA runtime classes, nine standard-library modules,
and fifteen enumeration or constant groups exposed by the installed library.
The `_HiddenModule` contract is retained because compatibility may depend on
members that are not displayed by ordinary object browsers.

Names such as `_B_str_Chr` and `_B_var_Chr` are type-library entry-point
names for typed VBA forms such as `Chr$` and `Chr`. Source spelling and
overload resolution belong to the future VBA language requirements; this set
preserves the inspected binary contract exactly.

| Requirement | Kind | Contract | Members |
| --- | --- | --- | ---: |
| `REQ-0069` | Class | [ErrObject](req-0069-err-object-api-contract.md) | 8 |
| `REQ-0070` | Class | [Collection](req-0070-collection-api-contract.md) | 5 |
| `REQ-0071` | Module | [Strings](req-0071-strings-api-contract.md) | 55 |
| `REQ-0072` | Module | [Conversion](req-0072-conversion-api-contract.md) | 25 |
| `REQ-0073` | Module | [FileSystem](req-0073-file-system-api-contract.md) | 20 |
| `REQ-0074` | Module | [DateTime](req-0074-date-time-api-contract.md) | 26 |
| `REQ-0075` | Module | [Information](req-0075-information-api-contract.md) | 15 |
| `REQ-0076` | Module | [Interaction](req-0076-interaction-api-contract.md) | 23 |
| `REQ-0077` | Module | [Math](req-0077-math-api-contract.md) | 12 |
| `REQ-0078` | Module | [Financial](req-0078-financial-api-contract.md) | 13 |
| `REQ-0079` | Module | [_HiddenModule](req-0079-hidden-module-api-contract.md) | 9 |
| `REQ-0080` | Enumeration | [VbVarType](req-0080-vb-var-type-api-contract.md) | 18 |
| `REQ-0081` | Enumeration | [VbMsgBoxStyle](req-0081-vb-msg-box-style-api-contract.md) | 20 |
| `REQ-0082` | Enumeration | [VbMsgBoxResult](req-0082-vb-msg-box-result-api-contract.md) | 7 |
| `REQ-0083` | Enumeration | [VbFileAttribute](req-0083-vb-file-attribute-api-contract.md) | 8 |
| `REQ-0084` | Enumeration | [VbStrConv](req-0084-vb-str-conv-api-contract.md) | 9 |
| `REQ-0085` | Enumeration | [VbDayOfWeek](req-0085-vb-day-of-week-api-contract.md) | 8 |
| `REQ-0086` | Enumeration | [VbFirstWeekOfYear](req-0086-vb-first-week-of-year-api-contract.md) | 4 |
| `REQ-0087` | Enumeration | [VbIMEStatus](req-0087-vb-imestatus-api-contract.md) | 20 |
| `REQ-0088` | Enumeration | [VbAppWinStyle](req-0088-vb-app-win-style-api-contract.md) | 6 |
| `REQ-0089` | Enumeration | [VbCompareMethod](req-0089-vb-compare-method-api-contract.md) | 3 |
| `REQ-0090` | Enumeration | [VbCalendar](req-0090-vb-calendar-api-contract.md) | 2 |
| `REQ-0091` | Enumeration | [VbDateTimeFormat](req-0091-vb-date-time-format-api-contract.md) | 5 |
| `REQ-0092` | Enumeration | [VbTriState](req-0092-vb-tri-state-api-contract.md) | 3 |
| `REQ-0093` | Enumeration | [VbCallType](req-0093-vb-call-type-api-contract.md) | 4 |
| `REQ-0094` | Enumeration | [Constants](req-0094-constants-api-contract.md) | 11 |

## Evidence Boundary

The type library establishes the callable surface and constant values. Detailed
string, conversion, date, file-system, financial, interaction, error, locale,
and host-dependent behavior requires requirements derived from `VBA.HLP` and
confirmed by executable reference probes.

# REQ-0179 — Source-visible VBA constant enumerations

## Requirement

The MP-0002 evaluator shall resolve the following case-insensitive `Vb*`
constant names to their type-library values as source-visible `Long`
expressions, and shall treat each as a reserved identifier so source cannot
redeclare it (`WFC0017`):

- `VbCompareMethod` (`REQ-0089`): `vbBinaryCompare`, `vbTextCompare`,
  `vbDatabaseCompare`;
- `VbVarType` subset (`REQ-0080`): `vbLong`, `vbString`, `vbBoolean`;
- `VbStrConv` (`REQ-0084`): `vbUpperCase`, `vbLowerCase`, `vbProperCase`,
  `vbWide`, `vbNarrow`, `vbKatakana`, `vbHiragana`, `vbUnicode`,
  `vbFromUnicode`;
- `VbTriState` (`REQ-0092`): `vbUseDefault`, `vbTrue`, `vbFalse`;
- `VbCallType` (`REQ-0093`): `vbMethod`, `vbGet`, `vbLet`, `vbSet`;
- `VbFileAttribute` (`REQ-0083`): `vbNormal`, `vbReadOnly`, `vbHidden`,
  `vbSystem`, `vbVolume`, `vbDirectory`, `vbArchive`, `vbAlias`;
- `VbMsgBoxResult` (`REQ-0082`): `vbOK`, `vbCancel`, `vbAbort`, `vbRetry`,
  `vbIgnore`, `vbYes`, `vbNo`;
- `VbDayOfWeek` (`REQ-0085`): `vbUseSystemDayOfWeek`, `vbSunday`, `vbMonday`,
  `vbTuesday`, `vbWednesday`, `vbThursday`, `vbFriday`, `vbSaturday`;
- `VbMsgBoxStyle` (`REQ-0081`): `vbOKOnly`, `vbOKCancel`, `vbAbortRetryIgnore`,
  `vbYesNoCancel`, `vbYesNo`, `vbRetryCancel`, `vbCritical`, `vbQuestion`,
  `vbExclamation`, `vbInformation`, `vbDefaultButton1`–`vbDefaultButton4`,
  `vbApplicationModal`, `vbSystemModal`, `vbMsgBoxHelpButton`, `vbMsgBoxRight`,
  `vbMsgBoxRtlReading`, `vbMsgBoxSetForeground`;
- `VbAppWinStyle` (`REQ-0088`): `vbHide`, `vbNormalFocus`, `vbMinimizedFocus`,
  `vbMaximizedFocus`, `vbNormalNoFocus`, `vbMinimizedNoFocus`;
- `VbFirstWeekOfYear` (`REQ-0086`): `vbUseSystem`, `vbFirstJan1`,
  `vbFirstFourDays`, `vbFirstFullWeek`;
- `VbCalendar` (`REQ-0090`): `vbCalGreg`, `vbCalHijri`;
- `VbDateTimeFormat` (`REQ-0091`): `vbGeneralDate`, `vbLongDate`, `vbShortDate`,
  `vbLongTime`, `vbShortTime`;
- General constants (`REQ-0094`): the integer `vbObjectError`, and the string
  constants `vbNullString`, `vbNullChar`, `vbCrLf`, `vbNewLine`, `vbCr`, `vbLf`,
  `vbBack`, `vbFormFeed`, `vbTab`, and `vbVerticalTab`, each resolving to its
  exact stored bytes.

## Scope

These members are exposed for source compatibility so VB and VBA code that
references them compiles and evaluates. They carry no runtime behavior beyond
their integer value; the functions and objects that consume them (for example
`MsgBox`, file-system enumeration, and date services) remain deferred with the
value types and host services the evaluator does not yet model.

## Verification

Unit tests print representative members of each enumeration and confirm the
reserved-name diagnostic for a redeclaration attempt.
`TC-MP0002-vba-constants-cli` covers a cross-enumeration sample through
`wfc --eval`.

## Traceability

This requirement implements the source-visible constant portions of `REQ-0080`,
`REQ-0081`, `REQ-0082`, `REQ-0083`, `REQ-0084`, `REQ-0085`, `REQ-0086`,
`REQ-0088`, `REQ-0089`, `REQ-0090`, `REQ-0091`, `REQ-0092`, `REQ-0093`, and
`REQ-0094`.

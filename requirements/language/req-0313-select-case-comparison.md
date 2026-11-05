# REQ-0313 — Select Case compares as Visual Basic 6 does

**Content type:** Project requirement

**Status:** Implemented

**Milestone:** MP-0010 — Distribution, Libraries and Command Line

**Source:** The owner's instruction that `Select Case` behave exactly as
Visual Basic 6 does (2026-11-04), measured with
`evidence/reference/select-case-probe`.

**Depends on:** REQ-0153 through REQ-0156

## Requirement

For every `Case` clause (a value, a `Case Is` relation or a `To` range) WFC
shall compare the selector with the Case value exactly as Visual Basic 6 does.
Those rules are not the rules of the `=`, `<` and `>` operators. They were
measured, not read from the documentation: about 6,700 comparisons were run
under Visual Basic 6.00.8176 and the answers are in
`evidence/reference/select-case-probe/vb6-reference.txt`.

**A typed selector** (a variable or expression that is not a Variant) converts
the Case value to its own type, as `CInt`, `CLng`, `CByte`, `CSng`, `CDbl`,
`CCur`, `CBool`, `CDate` or `CStr` would, and compares in that type:

- a number that is not whole is rounded to the nearest even number for an
  Integer, Long or Byte selector (`Case 5.4` matches 5, `Case 4.5` is 4);
- a String is read as a number (`Case "5"` matches 5); a String that is not a
  number is error 13;
- a number that does not fit the type is error 6 (`Byte` against `-1`, `Integer`
  against `70000` or a Date), but `CByte(True)` is 255;
- `Null` is error 94 (Invalid use of Null); `Empty` is 0 or `""`;
- a Boolean selector reads the Case value as `CBool` (`Select Case True` with
  `Case 5` matches) and a String selector reads it as `CStr` (so
  `Select Case "5"` with `Case 5` matches, and `"abc"` against a number is no
  match, not an error);
- a Single or Double selector against a Currency value compares as the
  floating-point type sees it (the Currency in 80-bit extended precision), so
  0.1 as a Double is a hair above 0.1 as a Currency; a Single selector against
  a Double constant or variable converts the Double to Single, against a Variant
  Double it does not.

**A Variant selector** (or a Variant Case value beside a Variant selector)
compares as Variants do:

- `Null` matches nothing and raises nothing, in any clause (`Case Null` too);
- `Empty` is 0 beside a number, a Boolean or a Date, and `""` beside a String;
- a String Variant beside a typed number, Boolean or Date is converted to that
  type when it can be; when it cannot it is the greater (no error);
- beside a typed String the Variant is compared as a String (`CStr`);
- two Variants order every number, Boolean and Date before every String
  (`Variant 5 = Variant "5"` is false), Strings by the text rules and numbers
  by value (a Currency beside another number compares as Currency, a Single
  beside a Double as Single).

The Boolean `True` is -1 and `False` is 0 for ordering. A text comparison
follows `Option Compare`; the comparison of a `To` range is the lower bound
with `>=` and then the upper with `<=`.

The first matching `Case` runs. A value list stops at the first match; the
selector is evaluated once; a Case value after a match, and the values of
later Case clauses, are not evaluated. Code that is skipped raises nothing.

An error found while comparing is a run-time error of the statement
(trappable with `On Error`); there is no diagnostic for the type of a Case
value. `WFC0053` and `WFC0060` are no longer produced by `Select Case`.

## Known differences

- WFC applies `Option Compare Text` to the whole program; Visual Basic applies
  it to the module.
- After an error in a Case value, `On Error Resume Next` continues in the Case
  body in Visual Basic and after the `End Select` in WFC.
- A typed Currency or Date is, in compiled code, told from a Variant by the way
  it was declared; an expression that merely yields one (a function result) is
  treated as typed in the interpreter only when it reads no Variant.

## Verification

**Method:** Test

- `TC-MP0002-select-case-probe-interpret` and
  `TC-MP0002-select-case-probe-native` generate the probe
  (`scripts/New-SelectCaseProbe.ps1`), run it under WFC and compare 6,672
  lines with `vb6-reference.txt` (the lines of the `Option Compare Text`
  module and the `Resume Next` line are left out for the reasons above).
- `tests/evaluator_tests.cpp` and corpus `203-skipped-code-and-booleans` cover
  the cases of the earlier requirements that changed (`Case 0 To "2"` is a
  match; `Select Case c > 1` with `Case 0, 1` matches `True`).

## Relationships

- **Derived from:** the owner's instruction of 2026-11-04
- **Revises:** REQ-0142, REQ-0153, REQ-0154, REQ-0155, REQ-0156
- **Conflicts with:** None known

## Implementation Record

`src/interpreter/select_case.cpp` (`Interpreter::select_case_order`) holds the
rules. The interpreter's `Select Case` and, through
`NativeEngine::select_case_test` and bit 10 of the `variant_binary` service
flags, the native code of `src/compiler/lower.cpp` use it.

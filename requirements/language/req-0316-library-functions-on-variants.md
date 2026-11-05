# REQ-0316 — The functions of the library answer as Visual Basic 6 does

**Content type:** Project requirement

**Status:** Implemented

**Milestone:** MP-0010 — Distribution, Libraries and Command Line

**Source:** The owner's instruction that WFC behave exactly as Visual Basic 6
does, measured with `evidence/reference/library-probe`.

**Depends on:** REQ-0315

## Requirement

The functions of the VBA library shall answer for an operand of any kind the
type, the value or the error that Visual Basic 6.00.8176 gives. The rules were
measured, not read from the documentation: 5,933 calls of 87 functions on 28
kinds of operand were run under Visual Basic 6 and the answers are in
`evidence/reference/library-probe/vb6-reference.txt` (the result is written as
its type name and text, `Null`, or `err<number>`).

- **Null.** `Chr`, `ChrW`, `Space`, `Hex$`, `Oct$`, `Sgn`, `Val`, `Str$`,
  `Choose`, `DateSerial`, `Replace` and `Join` of an array with a Null in it
  are error 94 (also in their `$` forms). `Str`, `Round`, `String` (a Null
  character), `InStr` (a Null String) answer Null. `Format` with a picture of
  digits, and `FormatNumber`, `FormatCurrency` and `FormatPercent` of a Null or
  an Empty answer the empty String.
- **Results.** `Asc`, `AscW`, `Sgn`, `Year`, `Month`, `Day`, `Hour`, `Minute`,
  `Second`, `Weekday` and `StrComp` give an Integer; `Val` a Double. `Abs`,
  `Int`, `Fix` and `Round` of a Byte are the Byte, of a Date the Date.
  `Len` and `LenB` of a variable that is not a String and not a Variant give
  its size in bytes (Integer 2, Long 4, Single 4, Double 8, Currency 8, Byte 1,
  Boolean 2, Date 8); `LenB` of a Variant is twice the length of its text.
- **A Date, a Boolean, an Empty or a String that is a number** is accepted
  where a number is wanted: `Sqr`, `Log`, `Cos`, `Atn`, `Hex`, `Oct`,
  `Chr`, `ChrW` (a negative code is the code plus 65536), `Space`, `Choose`,
  `IIf`, `Switch`, `Format` with a picture of digits, `FormatNumber` and its
  kin; and where a date is wanted (`Year` to `Weekday`, `DateAdd`, `DateDiff`,
  `DateSerial`, `CDate`): a String that is a number is that number of days, a
  Boolean is -1 or 0, an Empty 0. `IsDate("5")` is False. `DateSerial` of a
  year from -99 to -1 is 2000 plus it, a year beyond 9999 is error 6.
  `String(n, code)` takes the code modulo 256 (error 6 beyond an Integer).
- **`Str`** of a number below 1 has no zero before the point (` .1`); of a
  Date it is the date text, of a Boolean `True` or `False`, each with the
  leading space when the argument is not a Variant; of an Empty ` 0`.
  `IsNumeric` of a Byte is True.
- **`InStr`** with an empty pattern gives the start (0 when the String is
  empty); `Split` of the empty String with the empty delimiter is an array of
  one element; `Array` keeps a Byte.
- **`Option Compare`** belongs to the module that calls `InStr`, `InStrRev`,
  `StrComp`, `Replace`, `Split` and `Filter`, in the interpreter and in native
  code.

## Verification

**Method:** Test

`TC-MP0002-library-probe-interpret` and `-native` generate the probe
(`scripts/New-LibraryProbe.ps1`), run it under WFC and compare all lines with
`vb6-reference.txt`. The evaluator tests and corpus 203 changed where they
expected the older rules.

## Relationships

- **Derived from:** the owner's instruction of 2026-11-05
- **Revises:** REQ-0247 (Byte), the diagnostics WFC0021 and WFC0073 of the
  functions above (now Visual Basic's own answers)
- **Conflicts with:** None known

## Implementation Record

`Interpreter::measured_library_call` (`src/interpreter/library_rules.cpp`) runs
before the older code of `parse_function_call_impl`; the interpreter tells
which arguments are Variants as it does for the operators. The compiler marks
a call of a comparing function with the `Option Compare` of its module
(`emit_library_call`, a mark before the name that `variant_call` reads). The
small runtime for Windows 2000 does not have these library rules.

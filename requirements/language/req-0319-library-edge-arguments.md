# REQ-0319 — The library functions answer for edge arguments as Visual Basic 6 does

**Content type:** Project requirement

**Status:** Implemented

**Milestone:** MP-0010 — Distribution, Libraries and Command Line

**Source:** The owner's instruction that WFC behave exactly as Visual Basic 6
does, measured with `evidence/reference/argument-probe` and
`evidence/reference/case-probe`.

**Depends on:** REQ-0316

## Requirement

The functions of the VBA library that take several arguments shall answer for
zero, negative, fractional, Null, empty and out-of-range arguments what Visual
Basic 6.00.8176 gives. 3,511 calls were run under Visual Basic 6 (the answers
are in `evidence/reference/argument-probe/vb6-reference.txt`), and the
`UCase` and `LCase` of all 65,534 characters of the basic plane (the table is
`src/interpreter/vb_case_table.hpp`, made by `scripts/New-CaseTable.py` from
`evidence/reference/case-probe/vb6-reference.txt`).

- **Null.** A Null where `Mid` wants a start or a length (unless the text is
  Null too and the length is the Null), `Left`/`Right` a count, `String` a
  count, `InStr` a start or a compare, `InStrRev` a String, `Replace` a
  pattern, a count, a start or a compare, `Split` a String, a delimiter or a
  limit, `Join` a delimiter, `Filter` a match, `Round` a digit count,
  `Weekday` a first day, `DateAdd` a number, and `DateValue`, `TimeValue`,
  `MonthName` and `WeekdayName` their argument, is error 94. `StrConv` of a
  Null is Null; `Like` with a Null is Null.
- **`InStr`.** An empty String to search gives 0 and an empty pattern gives
  the start, before the start or the compare is looked at; otherwise a start
  below 1 is error 5, and a compare of 2 is error 5 (3 and above is text).
  `Replace` takes 2 and above as text.
- **Dates.** `DateSerial` moves the months into the year first, then maps a
  year of 0 to 49 to 20xx, of 50 to 99 to 19xx and of -99 to -1 to 1901 to
  1999; a date before year 100 or after year 9999 is error 5, an argument that
  is not an Integer error 6. `TimeSerial` is the plain number of seconds (it
  is not wrapped into a day). `DatePart` is an Integer. `DateAdd` cuts its
  number and gives error 5 outside the range of a Date. `CDate`, `DateValue`
  and `IsDate` read `13/1/2000` day first, two-digit years as `DateSerial`
  does, and a time of `5 PM`.
- **Numbers.** `CLng`, `CDbl`, `CSng` and `CCur` read the hexadecimal text
  `&HFFFF` as 65535 (`CInt` as -1). `Round` rounds its digit count to even.
  `Abs` of the smallest Integer is a Long and of the smallest Long a Double.
  A result of `Exp` below the smallest normal Double is 0.
- **Text.** `Like` with an open list (`[a`) or a range that goes down (`[c-a]`)
  is error 93; `StrConv` of an apostrophe does not start a word, vbWide,
  vbNarrow and unknown conversions are error 5, and vbUnicode puts a NUL
  after each character; `AscB` is a Byte.
- **Case.** `UCase`, `LCase`, `StrConv` and the text comparison use the case
  of every character as Visual Basic 6 gives it.

Not followed yet: `StrConv(s, vbFromUnicode)` gives a Byte array (Visual
Basic gives a String of the bytes); the lines are listed in
`evidence/reference/argument-probe/known-differences.txt`.

## Verification

**Method:** Test

`TC-MP0002-argument-probe-interpret` and `-native` and
`TC-MP0002-case-probe-interpret` and `-native` generate the probes
(`scripts/New-ArgumentProbe.ps1`, `scripts/New-CaseProbe.ps1`), run them under
WFC and compare all lines with the references.

# REQ-0318 — Format and its kin answer as Visual Basic 6 does

**Content type:** Project requirement

**Status:** Implemented

**Milestone:** MP-0010 — Distribution, Libraries and Command Line

**Source:** The owner's instruction that WFC behave exactly as Visual Basic 6
does, measured with `evidence/reference/format-probe`.

**Depends on:** REQ-0316

## Requirement

`Format`, `FormatNumber`, `FormatCurrency`, `FormatPercent` and
`FormatDateTime` shall answer for a value of any kind and a picture of any
family what Visual Basic 6.00.8176 gives. 3,105 calls (27 kinds of value, 77
pictures and the extra arguments) were run under Visual Basic 6; the answers
are in `evidence/reference/format-probe/vb6-reference.txt`.

- **Null** gives the empty String for every picture (and `FormatDateTime` of a
  Null too).
- **The kind of picture decides what a value is.** For a text picture (`@ & < >
  !`) a Boolean is `True` or `False` and a Date is its text; for any other
  picture a Boolean is -1 or 0, a Date is its serial and a String that is a
  number is that number. A numeric value with a date picture (`ddddd`,
  `Long Date`, `ttttt`, `w`, ...) is a date serial; a serial outside the range
  of a Date is error 6.
- **Text pictures.** The empty String gives the empty String; `!` keeps the
  last characters of a text that is longer than the picture.
- **Numeric pictures.** A value that rounds to zero in the places of its own
  section takes the zero section; `*x` is a fill character and shows nothing;
  a zero in a scientific picture shows its first digit (`0.E+0`).
- **Currency.** The negative amount of `Currency` and of `FormatCurrency` (by
  default) is in parentheses; an omitted argument of `FormatNumber`,
  `FormatCurrency` and `FormatPercent` takes the default of the system.
- **`Format`** takes up to four arguments (first day of the week and first week
  of the year); `y` of a negative serial counts from the day of its integer
  part.
- **`FormatDateTime`** with a style outside 0 to 4 is error 5 whatever the
  value, before the value is looked at.

## Verification

**Method:** Test

`TC-MP0002-format-probe-interpret` and `-native` generate the probe
(`scripts/New-FormatProbe.ps1`), run it under WFC and compare all lines with
the reference.

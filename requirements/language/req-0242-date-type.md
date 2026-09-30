# REQ-0242 — `Date` type, literals, and date/time functions

## Statement

- `Date` value type (an OLE serial: days since 1899-12-30 plus a time
  fraction) usable in `Dim`/`Static`/`Const`/parameters/returns/fields/arrays.
- Literals `#m/d/yyyy#`, `#yyyy-mm-dd#`, `#h:mm[:ss] [AM|PM]#`, combined
  date and time.
- Arithmetic: `Date ± number` is a `Date`; `Date - Date` is a `Double`;
  comparison against a `Date` or number by serial. Implicit Date ↔ numeric
  and Date → String assignment. Rendering is the US General Date form
  (`m/d/yyyy h:mm:ss AM`; date-only when the time is midnight; time-only
  when the date is zero).
- Functions: `Now`, `Date`, `Time` (and `$` forms), `Timer`, `Year`,
  `Month`, `Day`, `Hour`, `Minute`, `Second`, `Weekday`, `DateSerial`,
  `TimeSerial`, `DateValue`, `TimeValue`, `DateAdd`, `DateDiff`,
  `DatePart`, `IsDate`, `CDate`, `MonthName`, `WeekdayName`,
  `FormatDateTime`; `Format(date, style)` with named and custom patterns;
  `TypeName` (`Date`), `VarType` (7); numeric conversions see the serial.

## Scope

- US-locale rendering and parsing only; month-name date text, `Date`
  statements (`Date = x`, `Time = x`), `Mid`-style `DateDiff` first-day
  arguments (accepted, ignored), and `Format`'s `c`/`w`/`ww`/`q`/`y` tokens
  are not implemented. `Now`, `Date`, `Time`, `Timer` read the host clock
  and are therefore non-deterministic.

## Verification

`tests/evaluator_tests.cpp` (deterministic functions only);
`TC-MP0002-date-cli`.

# REQ-0320 — Statements, objects and errors behave as in Visual Basic 6

**Content type:** Project requirement

**Status:** Implemented

**Milestone:** MP-0010 — Distribution, Libraries and Command Line

**Source:** The owner's instruction that WFC behave exactly as Visual Basic 6
does, measured with `evidence/reference/statement-probe`.

**Depends on:** REQ-0319

## Requirement

Small programs (the snippets of `evidence/reference/statement-probe/snippets.txt`:
Collection and Dictionary, the Err object and error handling, loops and
branches, procedures, variables and literals, arrays, strings, dates, Rnd and
files) shall do what they do under Visual Basic 6.00.8176. 316 snippets were
run under Visual Basic 6 (the answers are in
`evidence/reference/statement-probe/vb6-reference.txt`); the rules that came
out of them:

- **The Err object.** `On Error` (every form), `Resume`, `Resume Next` and
  `Err.Clear` clear it; a procedure that ends while its handler runs clears
  it. An `On Error GoTo` inside a running handler does not arm a new handler.
  `Err.Raise` without a description or a source keeps the ones it has (and
  otherwise the standard text and the project name); a number beyond 65535 is
  error 5; a negative number without a description is "Automation error".
  `Err.Number`, `Err.Description`, `Err.Source`, `Err.HelpFile` and
  `Err.HelpContext` can be assigned. `Err.Source` of a run-time error is the
  name of the project. Error 35 reads "Sub or Function not defined".
- **Collection.** The key of `Add` must be a String (error 13 for a Null or a
  number); the items keep their type (an Integer stays an Integer); a
  `For Each` visits the items added by its body, and a Variant that ran
  through the items is Empty after the loop.
- **For.** A Variant counter has the type that Visual Basic gives to the
  three values (an Integer for `1 To 3`, a Double for text); the overflow of a
  counter is error 6; a String that is a number is a bound.
- **Literals.** `-32768` is a Long and `-2147483648` a Double (`32768` and
  `2147483648` are a Long and a Double literal).
- **Statements.** `Mid(s, start) = t` with a start beyond the text is error 5;
  a `String * n` starts as `n` characters of code 0; `Name .Member` inside a
  `With` passes the member to the procedure `Name`; an object assigned without
  `Set` is error 91; `Open` of an empty name is error 75; `Loc` of a
  sequential file counts blocks of 128 bytes from 1.
- **Conversions.** `CLng`, `CInt` and the others read the number of an error
  value (`CVErr(11)` is 11).
- **Rnd.** `Rnd(negative)` seeds from the bits of the Single and steps once;
  `Randomize n` mixes the high word of the Double into the seed and keeps its
  lowest byte (the algorithm of MSVBVM60, as `rtcRandomNext` has it), so a
  program that calls `Randomize` with the same number twice may not repeat its
  numbers.

## Verification

**Method:** Test

`TC-MP0002-statement-probe-interpret` and `-native` generate the probe
(`scripts/New-StatementProbe.ps1`), run it under WFC and compare all lines
with the reference.

# REQ-0315 — The arithmetic and logical operators work as Visual Basic 6 does

**Content type:** Project requirement

**Status:** Implemented

**Milestone:** MP-0010 — Distribution, Libraries and Command Line

**Source:** The owner's instruction that WFC behave exactly as Visual Basic 6
does, measured with `evidence/reference/arithmetic-probe`.

**Depends on:** REQ-0314

## Requirement

The operators `+ - * / \ Mod ^ & And Or Xor Eqv Imp`, and the unary `-` and
`Not`, shall give for two operands of any kinds the type, the value or the
error that Visual Basic 6.00.8176 gives. The rules were measured, not read from
the documentation: 16,920 operations on 35 kinds of operand were run under
Visual Basic 6 and the answers are in
`evidence/reference/arithmetic-probe/vb6-reference.txt` (the result is written
as its type name and its text, `Null`, or `err<number>`).

An operand is **typed** (not a Variant) or a **Variant** (a Variant variable,
`Null` or `Empty`). Empty counts as 0 or as `""`, and adopts the type of the
other operand (Integer when that is a Boolean, a Date or Empty too).

- **`+ - *`** give the wider type: Byte < Integer < Long; a Single with a Long
  is Double; Double beats Single; Currency beats Single and Double under `+`
  and `-`, and Single and Double beat it under `*`; a Date stays a Date under
  `+` and `-` (Date - Date and anything `*` Date are Double); a Boolean is an
  Integer. Two typed operands whose result does not fit are error 6; when an
  operand is a Variant the result is promoted instead (Byte, Integer, Long,
  Double). A result outside the dates is error 6 (Date first) or 5, or a Double
  beside a Variant.
- **A String** beside a number is read as a Double (error 13 if it is not a
  number); two Strings under `+` are joined; a String beside Empty under `+`
  is the String. A Null operand gives Null, but a String that is not a number
  is still error 13 (except under `+`).
- **`/`** gives a Double, or a Single when both operands are Singles or whole
  numbers and one is a Single. Dividing by zero is error 11 when both operands
  are typed and the dividend is not zero, error 6 otherwise.
- **`\` and `Mod`** round the operands to Long (banker's rounding, error 6 when
  they do not fit), divide by zero is error 11, and the result is Byte for two
  Bytes, Integer for operands that are Byte, Boolean or Integer, and Long
  otherwise. A String "True" is a Boolean when the other operand is a Variant.
- **`^`** gives a Double. Zero to a negative power and a negative number to a
  fraction are error 5; an overflow is error 6.
- **`And Or Xor Eqv Imp`** work on the bits of the operands rounded to Long and
  give Boolean for two Booleans, Byte for two Bytes, Integer for operands that
  are Byte, Boolean or Integer, and Long otherwise. With a Null operand the
  result is Null unless the other operand decides it: `0 And Null` is 0,
  `x Or Null` is x when x is not 0, `x Imp Null` is `Not x` when that is not 0,
  `Null Imp x` is x when x is not 0 (`Xor` and `Eqv` give Null).
- **`&`** gives a String; two Nulls give Null, one Null is the empty String.
- **Unary `-`**: a Byte or a Boolean gives an Integer, an Empty gives Integer
  0, a String a Double, a Date a Date; the minimum of an Integer or Long is
  error 6 when typed and promoted when a Variant. **`Not`**: a Boolean gives a
  Boolean, a Byte a Byte, an Integer an Integer, anything else a Long; Empty is
  -1.
- A Boolean `\` or `Mod` a typed String (a number or "True") is a Boolean
  (`\` gives the left Boolean again, `Mod` False): a quirk of Visual Basic.

A number below 1 is written in full while the zeros after the point and the
significant digits together fit the digits of the type (15 for a Double, 7 for
a Single), so `0.00001` and `0.0000000001` are written in full and
`1.23456789012345E-02` is not (this revises REQ-0268, which followed C's `%G`).
A date before the epoch counts its time up from midnight (`CDate(-1.5)` is
12/29/1899 12:00:00 PM).

Code that does not run computes nothing. An error found while computing is a
trappable run-time error of Visual Basic (`Err.Number`), not a diagnostic.

## Verification

**Method:** Test

`TC-MP0002-arithmetic-probe-interpret` and `-native` generate the probe
(`scripts/New-ArithmeticProbe.ps1`), run it under WFC and compare all 16,920
lines with `vb6-reference.txt`. `tests/evaluator_tests.cpp`, corpus 20 and 81,
`TC-MP0002-currency-cli` and the native tests 13 and 25 cover the cases of
earlier requirements that changed; `TC-MP0010-win2000-programs` keeps the small
runtime for Windows 2000 in step.

## Relationships

- **Derived from:** the owner's instruction of 2026-11-04
- **Revises:** REQ-0268 (how a small number is written), REQ-0247 (Byte in the
  arithmetic), the diagnostics WFC0007, WFC0008, WFC0009, WFC0019 and WFC0104
  of the arithmetic operators (now run-time errors 13, 11, 6, 13 and none)
- **Conflicts with:** None known

## Implementation Record

`Interpreter::arithmetic` and `Interpreter::unary_operation`
(`src/interpreter/arithmetic.cpp`, with the operand model of
`src/interpreter/operand.hpp` shared with REQ-0313 and REQ-0314). The
interpreter's expression levels tell which operand holds a Variant (as for the
comparisons); native code passes that in the flags of `variant_binary`
(`native_engine.cpp`). A Decimal, an array or an object still takes the older
`numeric_binary`, `integer_binary` and `logical_binary`. The small runtime for
Windows 2000 has the same rules in C (`ar_*` in `src/win2000/wfcw2k.c`) for the
types it has (no Currency or Date). `float_text` (`vb_text.hpp`) writes a
Double or Single for the interpreter, native code and the compiler.

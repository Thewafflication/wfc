# REQ-0246 — `Mid` statement, financial functions, `FormatNumber` family, `Partition`, `DoEvents`, `Debug.Print`

## Statement

- `Mid[$](var, start[, length]) = expr` overwrites part of a String variable
  without changing its length.
- `Pmt`, `FV`, `PV`, `NPer`, `IPmt`, `PPmt`, `NPV`, `IRR`, `SLN`, `SYD`,
  `DDB` (Double results; standard annuity formulas).
- `FormatNumber`, `FormatCurrency` (`$`, negatives in parentheses) and
  `FormatPercent` with digits and grouping arguments (US locale).
- `Partition`, `DoEvents` (no-op returning 0), and `Debug.Print`
  (evaluated, output discarded).

## Scope

`Rate`, `MIRR`, `Format*` leading-digit/parenthesis tri-state arguments
beyond True/False, annuity-due `IPmt` exactness for payment types other than
0, and Immediate-window capture are not implemented.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-misc-intrinsics-cli`.

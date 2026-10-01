# REQ-0282 — `Print` number spacing

## Statement

- With `EvaluationOptions::vb6_print_spacing` (set by the command line for project
  and `.bas` runs), `Print` and `Debug.Print` write a number (Integer, Long, Single,
  Double, Currency, Decimal, Byte) as a sign position, the digits, and a trailing
  space: ` 5 `, `-5 `, ` 1.5 `. Booleans, strings and dates carry no padding.
- Print zones (`,`) and `Tab`/`Spc` count the padded text, as in VB6.
- The default API and `wfc --eval` keep the compact rendering, so one-line
  snippets and the unit expectations stay readable.

## Verification

Every `tests/corpus/*.expected` file is generated with the option on; the compact
mode is covered by the evaluator unit tests and the `--eval` CLI tests.

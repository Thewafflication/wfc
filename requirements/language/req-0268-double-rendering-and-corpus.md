# REQ-0268 — VB number rendering, and the self-checking program corpus

## Statement

- A `Double` converts to text with **15 significant digits** and a `Single`
  with **7**, following C `%G` rules (exponent form below `1E-4` and from
  `1E15` / `1E7` up, written `1E+15` / `1E-05`), matching VB6's `Print`,
  `CStr`, `&` and `Format` default. This replaces shortest-round-trip
  rendering (`0.1 + 0.2` is `0.3`, `Sqr(2)` is `1.4142135623731`). The known
  VB6 `Rnd` fingerprint values already had seven digits; the other `Rnd`
  expectations that carried eight were corrected.
- `tests/corpus/` holds self-checking VB programs (`NN-name.bas`, or a
  `.vbp` project when one exists) with `.expected` stdout; CTest registers one
  `TC-MP0002-corpus-<name>` case per `.expected` file, run through
  `run_corpus_case.cmake`. Expected output was derived from VB6 semantics by
  hand, not from this evaluator.
- Fixed a defect the corpus exposed: assigning one UDT to another inside a
  not-taken branch (dry run) raised a type mismatch.
- `Err.Raise` accepts any non-zero `Long` (including `vbObjectError + n`).

## Scope

Rendering of `Decimal`/`Currency` is unchanged. The corpus is regression
evidence, not a substitute for retained VB6 reference probes.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-corpus-*`.

## Addendum (REQ-0269): corpus-driven fixes

- A numeric `String` beside a number (and numeric strings under `- * /`)
  converts to `Double` (`"5" + 10` is `15`, `"3" * "4"` is `12`); a
  non-numeric string is error 13. `String + String` still concatenates.
- Omitted argument slots (`Describe(, 7)`) take the `Optional` default; an
  omitted required argument is error 449.
- Robustness sweeps found and fixed crashes on error-subtype Variants and
  `Date` values reaching conversion functions (`CInt(CVErr(5))` now raises
  error 5; `CBool(#1/1/1900#)` works), `IsNumeric(Date)`, and ordering
  comparisons of arrays / error values (`WFC0018`).
- Known deviation recorded, not changed: unsuffixed integer literals are
  `Long` (`TypeName(5)` is `Long`; VB6 says `Integer`), per `REQ-0199`'s Scope.

Addendum: `Replace`, `InStr` and `InStrRev` accept an omitted optional slot
(`Replace(s, a, b, , 1)`, `InStr(, s, t)`); the slot takes its default
(start 1 / -1, count -1, `Option Compare` mode). Corpus `24-omitted-builtin-args`.

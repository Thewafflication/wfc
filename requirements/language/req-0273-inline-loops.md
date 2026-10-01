# REQ-0273 — Single-line loops and more mapped runtime errors

## Statement

- A `For`, `For Each`, `While` or `Do` header may be followed by `:` and body
  statements on the same line (`For i = 1 To 3: Print i: Next`), including a
  bare `Do:` closed by `Loop While|Until`.
- Library argument errors (negative `Left`/`Mid`/`Space` lengths, `InStr`
  start, `Replace` count, `Asc("")`, `Sqr` of a negative, `Round` digits,
  `QBColor`/`RGB` ranges, ...) are catchable as VB error 5 by
  `On Error`; a `LBound`/`UBound` dimension out of range is error 9.

## Verification

Corpus `25-inline-loops`.

Addendum: custom `Format` pictures accept a scientific section
(`0.00E+00`, `0.0e-0`): the mantissa picture, `E+`/`E-` and one or more exponent
digit placeholders. Corpus `26-format-scientific`.

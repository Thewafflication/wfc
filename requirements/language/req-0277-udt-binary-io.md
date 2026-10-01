# REQ-0277 — Fixed-length String in UDTs, `Len` of UDTs, `Get`/`Put` of UDTs and arrays

## Statement

- `Name As String * n` inside a `Type` (or class field) is a fixed-length
  string: it starts as `n` spaces and assignments truncate or pad to `n`.
- `Len(udt)` returns the UDT's size in bytes (fixed strings count `n`, a
  variable `String` counts a 2-byte descriptor plus its text, `Long` 4,
  `Integer` 2, `Double`/`Currency`/`Date` 8, ...). `Len(number)` returns the
  length of its text (`Len(42)` is 2).
- `Get`/`Put` accept a UDT variable, an array element (`items(i)`), a field
  path (`rec.sub.field`), or a whole array, in Binary and Random files.
  Fields are written in declaration order; variable strings carry a 2-byte
  length.

## Scope

Dynamic-array descriptors and Variant contents are not written.

## Verification

Corpus `34-udt-binary-io` and `33-calculator`.

# REQ-0256 — Binary and Random files, `Get`/`Put`/`Seek`

## Statement

`Open path For Binary|Random As #n [Len = recordLength]` (created if
missing), `Get [#]n, [position], variable`, `Put [#]n, [position], variable`,
`Seek #n, position`, and `Seek(n)`. Binary positions are 1-based byte
offsets; Random positions are 1-based records (default record length 128).
Supported variables: `Long` (4 bytes), `Integer` (2), `Byte` (1), `Single`
(4), `Double` (8), `Currency` (8), `Date` (8), `Boolean` (2), and `String`
(Binary: exactly `Len(variable)` bytes; Random: a 2-byte length prefix, or
the fixed length for `String * n`). Random `Put` pads the record with zeros;
`EOF`/`LOF`/`Loc` work on these modes. Reading past the end is error 62.

## Scope

UDT, Variant, and array variables for `Get`/`Put`, `Lock`/`Unlock`, and
`Seek` past the end for Sequential modes are not implemented.

## Verification

`tests/evaluator_tests.cpp` (temp-file round trip).

# REQ-0247 — `Byte` type

## Statement

`Byte` (0–255) is a value type usable wherever `Integer`/`Long` are:
`Dim`/`Static`/`Const`/parameters/returns/fields/arrays. `CByte` returns a
`Byte`; `TypeName` is `Byte`, `VarType` is 17. Assigning a value outside
0–255 (after rounding) raises overflow (`WFC0009`, error 6). `Not` and
bitwise `And`/`Or`/`Xor` on two Bytes yield a Byte.

## Scope

Deliberate deviation: arithmetic (`+ - * / \ Mod`, unary minus) on a Byte
widens to `Long` (real VB6 keeps `Byte + Byte` a Byte and overflows at 255);
the result narrows back, with the range check, on assignment to a Byte
variable. Byte arrays passed to `Get`/`Put`, `StrConv` byte conversions, and
`For` with a Byte control variable are not covered.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-byte-cli`. The former test asserting
`Dim value As Byte` was rejected now uses an unknown type name.

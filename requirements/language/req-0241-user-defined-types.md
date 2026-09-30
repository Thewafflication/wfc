# REQ-0241 — User-defined types (`Type ... End Type`)

## Statement

`[Public|Private] Type Name` ... `End Type` declares a record type whose
members are declared `member As Type`. A variable, nested member, array
element (fixed-size arrays), or parameter declared `As Name` holds its own
value: `b = a` copies all members (deep for nested types), a `ByVal`
parameter receives a private copy, and members are read/written with
`var.member`, `var.inner.member`, and `arr(i).member`.

## Implementation note

A UDT is registered as a class (`ClassDef::is_udt`) whose public fields are
the members; variables are eagerly instantiated (no `Set`), and assignment
copies instead of aliasing. This also enables `arr(i).member = v` for
class-typed arrays.

## Scope

Not covered: `ReDim` of a UDT array (elements stay `Nothing`), arrays as
members, fixed-length `String * n` (the length is ignored), `LSet`, `Len`/
`LenB` of a UDT, `Is`-comparison restrictions, and `Type` inside a class
module or procedure. Function results of UDT type and `Variant` holding a
UDT share rather than copy.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-udt-cli`.

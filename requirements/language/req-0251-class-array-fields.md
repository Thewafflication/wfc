# REQ-0251 — Array fields in class modules

## Statement

A class module may declare array fields: `Private items() As Long`
(dynamic, unallocated until `ReDim`), `Public grid(1 To 3) As Long`, and
multi-dimensional `Public cells(1 To 2, 1 To 2) As Double`, of any scalar,
`Variant`, `Object`, or class element type (UDT elements are instantiated
per slot). Members use them like any array (`items(n)`, `ReDim Preserve`,
`UBound`, `For Each`); outside the class a `Public` array field is read and
written as `obj.field(i) = v` / `obj.field(i)`, and passed whole as
`obj.field`.

## Scope

`Erase` and `ReDim` through `obj.field` from outside the class, and array
fields of UDT types declared inside `Type ... End Type`, are not supported.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-class-array-fields-cli`.

# REQ-0192 — Error message function subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0069, REQ-0072, REQ-0140, and REQ-0160

## Requirement

The MP-0002 evaluator shall recognize case-insensitive `Error` and `Error$`
with zero or one argument. With no argument, or with error number zero, it shall
return an empty String because the evaluator does not yet retain an `Err`
object. With a `Long` from 1 through 65535, it shall return the VBA description
for the selected stable catalog below, or `Application-defined or
object-defined error` when the valid number is not defined by this subset.

The stable catalog is 5, 6, 7, 9, 11, 13, 28, 53, 70, 76, and 91. Another
current value type shall fail with `WFC0073`; a number outside 0 through 65535
shall fail with `WFC0101`; more than one argument shall fail with `WFC0072`.

## Scope

This increment does not implement mutable `Err` state or host-specific error
descriptions. Once `Err` exists, the zero-argument form must return its most
recent description instead of always returning the no-error empty String.

## Verification

Unit tests cover every selected catalog message, both empty forms,
undefined-number fallback, both valid-range boundaries, type/range errors, and
arity. `TC-MP0002-error-function-cli` verifies both names and the fallback
through `wfc --eval`.

## Traceability

This requirement partially implements `REQ-0072` and records its dependency on
the future `REQ-0069` Err-object implementation.

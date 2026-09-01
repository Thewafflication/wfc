# REQ-0191 — MacID four-byte conversion

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0072, REQ-0140, and REQ-0177

## Requirement

The MP-0002 evaluator shall recognize the case-insensitive `MacID` intrinsic
with exactly one `String` argument. The argument shall contain exactly four
bytes. `MacID` shall pack those bytes in display order from the most-significant
through least-significant byte of the returned 32-bit `Long`.

An argument of another current value type shall fail with `WFC0073`; a String
whose byte length is not four shall fail with `WFC0100`; wrong arity shall fail
with `WFC0072`.

## Scope

This implements the pure conversion used for Macintosh resource types, file
types, application signatures, and Apple Events. It does not add Macintosh host
services or claim that `Dir`, `Kill`, `Shell`, or `AppActivate` can consume the
result on Windows. The current evaluator's byte-string model defines the
four-character subset as exactly four bytes.

## Verification

Unit tests cover known `TEXT`, `XLS8`, and `ABCD` packings plus type, length,
and arity failures. `TC-MP0002-macid-cli` verifies known codes through
`wfc --eval`.

## Traceability

This requirement partially implements `REQ-0072`.

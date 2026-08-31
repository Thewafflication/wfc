# REQ-0177 — Byte-string function subset

## Requirement

The MP-0002 evaluator shall recognize case-insensitive `LenB`, `AscB`, and
`ChrB`/`ChrB$` intrinsics with exactly one argument:

- `LenB(String)` shall return the number of stored bytes;
- `AscB(String)` shall return the unsigned value of the first stored byte and
  shall reject an empty String with `WFC0077`;
- `ChrB(Long)` and `ChrB$(Long)` shall return the one-byte String corresponding
  to values 0 through 255 and reject other values with `WFC0078`.

Wrong arity shall fail with `WFC0072`, and wrong argument types shall fail with
`WFC0073`.

## Scope

The current evaluator stores Strings as byte sequences and limits source text
semantics to its deterministic ASCII-compatible subset. These functions expose
that existing representation; they do not claim DBCS code-page behavior or
general Unicode/BSTR byte-layout equivalence.

## Verification

Unit tests cover byte length, first-byte conversion, round trips for values 0
and 255, and range rejection. `TC-MP0002-byte-string-cli` covers all three
functions through `wfc --eval`.

## Traceability

This requirement partially implements `REQ-0071`.

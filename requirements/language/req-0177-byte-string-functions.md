# REQ-0177 — Byte-string function subset

## Requirement

The MP-0002 evaluator shall recognize the case-insensitive byte-string
intrinsics below:

- `LenB(String)` shall return the number of bytes in the String's UTF-16LE form
  (twice `Len`, as in VB6);
- `AscB(String)` shall return the unsigned value of the first byte of that form
  and shall reject an empty String with `WFC0077`;
- `ChrB(Long)` and `ChrB$(Long)` shall return the one-byte String corresponding
  to values 0 through 255 and reject other values with `WFC0078`.
- `LeftB`/`LeftB$`, `RightB`/`RightB$`, `MidB`/`MidB$`, and `InStrB` shall use
  the same one-based slicing/search rules as their character counterparts,
  with positions and lengths measured in bytes of the UTF-16LE form.

Wrong arity shall fail with `WFC0072`, and wrong argument types shall fail with
`WFC0073`.

## Scope

Strings are UTF-16 code-unit sequences (stored as UTF-8). The B functions view
a String as its UTF-16LE bytes, as VB6 does, so `LenB("abc")` is 6 and
`LeftB("WFC", 2)` is `"W"`. A lone byte (from `ChrB` or an odd-sized slice) is
held as a private-use half unit U+F700+byte; two touching half units merge into
one unit on concatenation, so `ChrB(65) & ChrB(0)` equals `"A"`, and `Print`
shows a half unit as its raw byte. DBCS code pages are not modelled.

## Verification

Unit tests cover byte length, first-byte conversion, round trips for values 0
and 255, range rejection, slicing, and search. `TC-MP0002-byte-string-cli` and
`TC-MP0002-byte-slice-cli` cover the byte functions through `wfc --eval`.

## Traceability

This requirement partially implements `REQ-0071`.

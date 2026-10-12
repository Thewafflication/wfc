# REQ-0307 — Calling `Declare`d DLL routines

**Content type:** Project requirement

**Status:** Proposed

## Statement

- **Signature.** The `Lib`, `Alias` and parameter list of a `Declare` line are
  kept: for each parameter `ByVal` or `ByRef` and its type (`Long`, `Integer`,
  `Byte`, `Boolean`, `String`, a user-defined type) and the result type.
- **Call.** On Windows a routine that is not emulated is called in its DLL by
  name (the `Alias`, or the declared name, then the same with an `A` suffix),
  with the platform's calling convention. Up to twelve parameters are passed as
  integer-sized values.
  - `ByVal` `Long`, `Integer`, `Byte` and `Boolean` pass their value; a `ByRef`
    one passes a pointer to a temporary that is copied back into the caller's
    variable.
  - A `String` is passed as a NUL-terminated ANSI buffer; a `ByRef` string is
    copied back at its full length (the callee's NUL and what follows stay in
    the string, as in VB).
  - A user-defined type is passed as a pointer to its fields in declaration
    order with natural alignment (`Long`, `Integer`, `Byte`, `Single`,
    `Double` fields) and copied back.
  - The result `Long`, `Integer`, `Byte` or `Boolean` becomes the function value.
- **Errors.** A DLL that cannot be loaded raises 48 ("Error in loading DLL");
  a missing export, an unsupported parameter (`Single`, `Double`, `Any`,
  `Variant`, fixed-length strings, arrays) or more than twelve parameters
  raises 453.
- **Other platforms.** The call always raises 453.

## Known limits

Floating-point parameters need a different calling sequence on x64 and ARM64
and are not supported; Unicode (`W`) entry points are not reached by an `A`
declaration; callbacks (`AddressOf`) are not supported.

## Verification

Corpus `199-declare-dll-calls` (a cursor query with a user-defined type, a
`ByVal` string, a `ByRef` buffer, a missing export and a missing DLL).

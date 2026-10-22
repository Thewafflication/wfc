# ADR-0009: Variants, Objects and the Form Host in Native Programs

**Content type:** Architecture decision record

**Status:** Proposed (implementation started on the owner's instruction, 2026-10-22)

**Date:** 2026-10-22

## Context

`ADR-0008` keeps Variants, objects and the form and control host in the C++
runtime and reaches them through the entry table. The first native increments
(MP-0009 #1 to #12) cover the typed core of the language: integers, floating
point, strings, arrays, user-defined types, error handling, `Static`,
`Option Base`. The corpus games use more: Variants and late-bound objects
(`For Each c In frm.Controls`, `TypeName(c)`, `c.Font.Name = ...`), class
modules, collections, forms and controls.

The interpreter already implements the semantics of all of them on its own value
type, `wfc::detail::Value` (a `std::variant` of Integer, String, Boolean,
Double, Single, Currency, Decimal, Empty, Null, Int16, Nothing, arrays, object
instances, Date, Byte and Error values). Its operators, conversions, formatting
and the member access of controls, forms and the built-in classes are one
implementation, tested by the whole corpus. Writing a second implementation for
the native runtime would double the work and let the two drift.

## Decision

1. **A Variant is a handle to an immutable, reference-counted box that holds a
   `Value`.** The null handle is `Empty`. A Variant variable is one 8-byte slot,
   like a String, with the same ownership rules: a variable owns its handle, a
   function result transfers ownership, temporaries are released at the end of
   the statement. Copying a Variant shares the box. Boxes are never changed
   after they are made, so sharing is safe.
2. **One engine implements the semantics.** The native runtime owns one
   `wfc::detail::Interpreter` (the engine) and reaches it through a facade class
   `NativeEngine` (a friend of `Interpreter`, `src/interpreter/native_engine.*`).
   The facade offers value-level operations: arithmetic and comparison
   operators, conversions to and from the typed values of compiled code,
   `TypeName`, `VarType`, formatting for `Print`, member get, set and call on
   objects. An error the engine reports becomes a pending native error with the
   VB error number and text the interpreter would give
   (`Interpreter::runtime_error_number`). The interpreter itself is not changed
   except for the friend declaration and small accessors.
3. **Typed values cross to the Variant world at the edges.** Compiled code keeps
   typed values unboxed. Assigning a typed value to a Variant, passing it as a
   Variant argument or storing it in a Variant array boxes it; reading a Variant
   into a typed variable converts through the engine (error 13 on a mismatch).
4. **Objects are Variant boxes too.** An object reference (`Object`, a class
   name, `Control`, `Form`, `Collection`) is a handle to a box holding an
   `ObjectInstance` (or `Nothing`). Member access that is late bound goes
   through the engine's object interface by name (`get`, `set`, `call`), so
   controls, forms, `Screen`, `App`, `Err` and `Collection` behave exactly as in
   the interpreter. Compiled class modules (a later increment) are objects of
   the same kind whose members are compiled procedures reached through a member
   table.
5. **The form host stays shared.** Form and control definitions (`.frm` design
   data) are loaded by the same form runtime; the code of the form is compiled,
   and its event procedures are registered in a table the form runtime calls
   instead of interpreting source text. The program's event loop is the form
   runtime's.
6. **No second interpreter of source text.** The native program does not parse or
   interpret VB source at run time; it links the shared C++ runtime (engine
   included) as a library. Packaging (`MP-0009` stage 7) ships that runtime with
   the native payload.

## Alternatives considered

- **A native tagged 16-byte Variant with its own arithmetic.** Faster, but a
  second implementation of every operator, conversion and format; rejected until
  the corpus passes.
- **Calling the interpreter on VB source for Variant code.** Rejected: it is the
  embedded interpreter this milestone removes.

## Consequences

- Variant code is slower than typed code (a box per value); typed code is
  unaffected. Optimization (small-integer and double caching, unboxed locals) is
  possible later without changing the semantics.
- Corpus parity of Variant behavior comes for free, and so do its defects: a bug
  fixed in the interpreter is fixed natively.
- The runtime API version (`kRuntimeApiVersion`) changes with the entry table.
- Native tests compare native output with interpreter output, as before.

## References

- `ADR-0008` native compilation pipeline
- `planning/MP-0009-native-compilation.md`, `planning/work-log-mp-0009.md`

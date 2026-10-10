# REQ-0288 — Type-library references and early binding

**Content type:** Project requirement

**Status:** Accepted (Windows)

## Statement

Each `Reference=` line of a `.vbp` (`*\G{GUID}#major.minor#lcid#path#name`)
names a type library, loaded from the registry (falling back to `path`). For
the identifiers a program mentions:

- Its coclasses, interfaces and dispinterfaces become class names, both
  unqualified (`DOMDocument60`) and qualified by the library name
  (`MSXML2.DOMDocument60`). A user class or an earlier reference wins an
  unqualified clash.
- `Dim x As Lib.Class`, `Dim x As New Lib.Class` and `Set x = New Lib.Class`
  create the coclass through COM (`REQ-0286`). An interface name can be used as
  a variable type but not created with `New` (error 429). Assigning any COM
  object to a variable of an imported class is accepted.
- The members of its enumerations and module constants are global constants
  (`NODE_ELEMENT`, `adOpenStatic`), except where a built-in constant already has
  the name.
- A library name qualifier on a constant or function (`Excel.xlUp`) is
  dropped.

Without a project (a bare `.bas`) there are no references. A reference that
cannot be loaded is ignored; programs may still use `As Object`.

## Known limits

No event sinks (`WithEvents` on a COM object), no `Implements` of a COM
interface, and no check that a member exists before the call runs (calls are
still late-bound).

## Verification

`TC-MP0002-corpus-147-typelib-binding` (the Microsoft XML 6.0 and Windows
Script Host libraries).

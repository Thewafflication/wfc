# REQ-0283 — Multi-module projects and class/module integration

## Statement

- **Separate module namespaces.** The standard modules of a `.vbp` keep their own
  module-level names. When two modules declare the same name (a procedure,
  variable, constant, `Type`, `Enum` type, or `Declare`), the loader renames it
  inside each declaring module (`Name__Module`), so private helpers and constants
  may repeat across modules. `Module.Name` resolves to the named module's
  declaration; an unqualified reference to a colliding name that exactly one other
  module exports resolves to that export; a class module's own members shadow the
  modules' names. A renamed `Declare` without an `Alias` gains an `Alias` for its
  original export. `Enum` members are not renamed.
- **Option statements.** Each module's `Option` lines are removed (keeping
  line numbers). `Option Base 1` and `Option Compare Text|Binary` are merged into
  one header when any module has them. `Option Explicit` applies only inside the
  modules (and class modules) that declare it, so a module without it may still
  use undeclared variables.
- **Classes and modules.** A class member can call standard-module procedures and
  read or write module-level variables and constants. `Private Declare` and
  `Private Type` statements are accepted in class modules; `Attribute
  VB_PredeclaredId = True` gives a class a global default instance named after the
  class; `As Class.Enum` / `As Module.Enum` are accepted as type names.
- **Project metadata.** `Title`, `ExeName32`, `MajorVer`, `MinorVer`,
  `RevisionVer` and the `Version*` strings of the `.vbp` are the `App` object's
  `Title`, `EXEName`, `Major`, `Minor`, `Revision`, `CompanyName`, `ProductName`,
  `FileDescription`, `Comments` and `LegalCopyright`.
- **Label rule.** An indented `Name:` whose name is a procedure is a call followed
  by another statement, not a label.

## Known limits

`VB_GlobalNameSpace` classes are not given global members.

## Verification

`TC-MP0002-corpus-114-multi-module`, `-108-app-metadata`, `-121-qualified-enum-type`,
`-122-predeclared-class`, `-124-chained-objects`, `-130-class-declare`,
`-131-class-private-type`, `-126-recursive-descent-calc` and
`-139-per-module-explicit`.

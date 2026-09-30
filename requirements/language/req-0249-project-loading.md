# REQ-0249 — Loading `.vbp`, `.bas`, and `.cls` files

## Statement

`wfc project.vbp` (or `wfc a.bas b.bas c.cls ...`) loads real VB6 project
files. A `.vbp`'s `Module=Name; path.bas` and `Class=Name; path.cls` entries
are read relative to the project directory (backslashes accepted), all
standard modules are concatenated into one program, and each class module is
registered by its `Attribute VB_Name` (or the `Class=` name, or the file
stem). File-format header lines (`VERSION ... CLASS`, `BEGIN ... END`,
`Attribute ...`) are blanked, preserving line numbers. `Startup="Sub Main"`
(the default) appends `Call Main` when a module declares `Sub Main`. `Option
Explicit` is accepted at the top of class modules.

## Scope

Form, UserControl, PropertyPage, and Designer entries and non-`Sub Main`
startup objects are rejected with a clear message. Module-qualified names
(`Module1.Proc`), module-private scoping, `Reference=`/`Object=` resolution,
resource files, and conditional-compilation arguments from the project file
are not implemented; standard modules share one namespace.

## Verification

`TC-MP0002-vbp-project-cli` loads `tests/fixtures/project/Hello.vbp`
(module + class with CRLF file headers).

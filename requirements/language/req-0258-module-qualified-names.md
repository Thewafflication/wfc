# REQ-0258 — Module-qualified names

## Statement

When a program (or `.vbp` project) contains `Attribute VB_Name = "Util"`
module headers, `Util.Name` resolves to `Name` — procedures
(`Util.Twice(3)`), module variables and constants (`Util.Counter = 4`) —
unless a variable called `Util` is in scope. The `.vbp` loader keeps the
`VB_Name` attribute of standard modules for this purpose.

## Scope

All standard modules still share one namespace: two modules declaring the
same procedure/variable name collide, and `Private` module members are not
hidden from other modules. Class members are unaffected.

## Verification

`TC-MP0002-vbp-modules-cli` (two-module project).

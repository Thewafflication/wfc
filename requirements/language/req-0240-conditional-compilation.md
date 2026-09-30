# REQ-0240 — Conditional compilation

## Statement

`#Const Name = expr`, `#If expr Then`, `#ElseIf expr Then`, `#Else`,
`#End If` are resolved before parsing: directive lines and lines in inactive
branches are blanked (byte offsets and line numbers are preserved). Applies
to the main program and every class module. Expressions support integers,
`True`/`False`, `#Const` names, the predefined `Win32`/`VBA6`/`VBA7`
(True), `Win16`/`Mac` (False), `VBAVer`, `Not`/`And`/`Or`/`Xor`,
comparisons, `+ - * Mod`, parentheses and unary minus. Undefined names are 0.
Nesting is supported. Malformed directives report `WFC0310`.

## Scope

String/floating-point conditional constants, `Is`/`Like`, and intrinsic
functions in conditional expressions are not supported.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-conditional-compilation-cli`.

# REQ-0261 — Line continuation

## Statement

A line whose last non-blank character is an underscore preceded by white
space (outside a string literal or comment) continues on the next line
(` _`). Implemented as a source pass that blanks the underscore and the line
break, so byte offsets and diagnostics positions are unchanged. Applies to
the main program and every class module.

## Scope

Continuation inside `#If` directive lines is handled after directives are
resolved; a comment ending in `_` does not continue.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-line-continuation-cli`.

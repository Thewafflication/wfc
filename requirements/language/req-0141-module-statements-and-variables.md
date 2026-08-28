# REQ-0141 — Module statement and variable subset

**Content type:** Project requirement

**Status:** Proposed

**Source:** MP-0002 core VB/VBA language execution objective

## Scope

This requirement applies to the second MP-0002 language increment accepted by
`wfc --eval`. It composes REQ-0140 expressions into a small sequential module
program.

## Requirement

WFC shall execute one or more statements separated by a Windows or Unix line
ending or by `:`. Apostrophe comments shall extend to the next line ending.
Keywords and variable names shall be case-insensitive.

The supported statements shall be:

- `Dim <name> As Long` and `Dim <name> As String`, which create module variables
  initialized to `0` and the empty string respectively;
- `<name> = <expression>` and `Let <name> = <expression>`, which assign a value
  of the variable's declared type; and
- `Print <expression>`, which evaluates REQ-0140 expressions extended with
  declared variable references and appends one output line.

Variable names shall begin with an ASCII letter and continue with ASCII letters,
digits, or underscores; a keyword in this subset shall not be accepted as a
variable name. A variable shall be declared before use. Duplicate case-insensitive
declarations and assignment between `Long` and `String` shall fail rather than
silently convert.

The evaluator shall produce stable diagnostics for an invalid statement,
missing variable name or supported type, duplicate declaration, missing
assignment operator, undeclared variable, and assignment type mismatch.
Reserved variable names shall also produce a stable diagnostic.

## Rationale

Sequential state is the smallest useful program model after expression
evaluation. Exact-type assignment deliberately postpones VB6 Variant and
implicit-conversion semantics until their representation and reference corpus
are defined.

## Verification

**Method:** Test

`TC-MP0001-evaluator` shall retain all earlier cases and additionally cover
typed defaults, assignment forms, case-insensitive lookup, statement separators,
comments, multiple output lines, and every new diagnostic class.
`TC-MP0002-program-cli` shall execute declaration, arithmetic assignment, and
output through the product command.

## Relationships

- **Derived from:** MP-0002 core VB/VBA language execution objective
- **Depends on:** REQ-0140
- **Conflicts with:** None known

## Tailoring

This subset does not yet include `Option Explicit`, implicit declarations,
Variants, type characters, declaration lists, constants, arrays, procedure
scope, implicit conversions, labels, or control flow. Those behaviors require
later requirements and reference evidence.

## Implementation Record

Implemented by `evaluate_program` and its sequential interpreter in
`src/evaluator.cpp`, with unit and CLI coverage in the named CTest cases.

# REQ-0136 — Minimal source-to-result vertical slice

**Content type:** Project requirement

**Status:** Proposed

**Source:** MP-0001 feasibility objective

## Requirement

WFC shall accept a documented minimal VB source statement, parse it without
case sensitivity, produce an observable result through a product executable,
and return stable diagnostics and a failing process status for malformed input.
The initial supported statement is `Print` followed by one string literal;
doubled quotation marks shall represent a quotation mark in the value.

## Rationale

A deliberately narrow executable path proves source handling, semantics,
diagnostics, library boundaries, command dispatch, and automated verification
without implying broad VB compatibility.

## Verification

**Method:** Test

`TC-MP0001-evaluator` shall cover accepted spellings, whitespace, empty and
escaped literals, and each defined diagnostic class. `TC-MP0001-cli` shall run
the product executable and match its output. Both tests shall pass on a native
host architecture.

## Relationships

- **Derived from:** MP-0001 slice stage
- **Depends on:** REQ-0133, REQ-0134, and REQ-0135
- **Conflicts with:** None known

## Tailoring

This syntax is an MP-0001 feasibility subset, not a complete VB6 `Print`
statement or compatibility claim. Full grammar and output semantics belong to
MP-0002 requirements and reference evidence.

## Implementation Record

Implemented by `wfc_frontend`, the `wfc --eval` command, and the two named CTest
tests. The current x86 and x64 host executions pass.

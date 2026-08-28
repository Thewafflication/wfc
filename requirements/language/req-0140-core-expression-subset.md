# REQ-0140 — Core expression execution subset

**Content type:** Project requirement

**Status:** Proposed

**Source:** MP-0002 core VB/VBA language execution objective

## Scope

This requirement applies to the first MP-0002 expression increment accepted by
the `wfc --eval` command. It extends, but does not replace, the MP-0001
string-literal vertical slice.

## Requirement

WFC shall evaluate a single `Print` statement whose expression contains:

- string literals with doubled-quotation escaping;
- decimal integer literals in the signed 32-bit range;
- parentheses and unary `+` and `-`;
- integer `+`, `-`, `*`, `\\`, and case-insensitive `Mod`; and
- `&` concatenation of string and integer results.

Multiplication, integer division, and `Mod` shall bind more tightly than
addition and subtraction. Arithmetic shall bind more tightly than
concatenation. Parentheses shall override normal precedence.

The evaluator shall reject malformed expressions, missing closing parentheses,
out-of-range literals, non-integer arithmetic operands, division by zero, and
arithmetic overflow with stable diagnostics and a failing process status.

## Rationale

Typed primary values and precedence-aware expression evaluation are the first
reusable language core beyond the feasibility parser. The narrow value and
operator set keeps unverified VB6 conversions, floating-point behavior, and
locale formatting outside the compatibility claim.

## Verification

**Method:** Test

`TC-MP0001-evaluator` shall retain the MP-0001 cases and cover every expression
form, precedence override, mixed concatenation, and diagnostic class added by
this requirement. `TC-MP0002-expression-cli` shall verify the complete command
path and observable output.

## Relationships

- **Derived from:** MP-0002 core VB/VBA language execution objective
- **Depends on:** REQ-0135 and REQ-0136
- **Conflicts with:** None known

## Tailoring

This is not a claim for floating-point or date literals, identifiers,
variables, calls, comparisons, logical operators, implicit VBA conversions, or
complete VB6 `Print` formatting. Those behaviors require later requirements and
reference evidence.

## Implementation Record

Implemented by the precedence parser and typed evaluator in
`src/evaluator.cpp`, with unit and CLI coverage in the named CTest cases.

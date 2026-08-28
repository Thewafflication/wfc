# REQ-0142 — Boolean and comparison expression subset

**Content type:** Project requirement

**Status:** Proposed

**Source:** MP-0002 core VB/VBA language execution objective

## Scope

This requirement applies to the third MP-0002 language increment accepted by
`wfc --eval`. It supplies the typed condition expressions needed before
conditional and loop statements enter scope.

## Requirement

WFC shall support the case-insensitive Boolean literals `True` and `False`,
`Dim <name> As Boolean` with a `False` default, exact-type Boolean assignment,
and `True` or `False` output formatting.

Expressions shall support:

- `=`, `<>`, `<`, `<=`, `>`, and `>=` between two `Long` values or two `String`
  values;
- `=` and `<>` between two Boolean values;
- unary `Not`; and
- binary `And`, `Or`, `Xor`, `Eqv`, and `Imp` between Boolean values.

String ordering shall use deterministic case-sensitive byte ordering for this
increment. Comparisons shall bind less tightly than arithmetic and `&` but more
tightly than logical operations. From highest to lowest, logical precedence
shall be `Not`, `And`, `Or`, `Xor`, `Eqv`, and `Imp`.

Mixed-type comparisons and Boolean ordering shall produce diagnostic
`WFC0018`. A logical operator with a non-Boolean operand shall produce
diagnostic `WFC0019`. Boolean concatenation shall remain outside the REQ-0140
`String`/`Long` boundary and produce diagnostic `WFC0020`.

## Rationale

Typed Boolean conditions and explicit precedence complete a reusable expression
layer for later control-flow work. Strict operands avoid silently inventing the
Variant coercion and integer bitwise rules that still require reference probes
and a broader value representation.

## Verification

**Method:** Test

`TC-MP0001-evaluator` shall retain the earlier corpus and cover Boolean defaults,
literals, assignments, output, every comparison family, every logical operator,
precedence, and all new diagnostic classes. `TC-MP0002-boolean-cli` shall
exercise declaration, arithmetic comparison, assignment, and logical output
through the product command.

## Relationships

- **Derived from:** MP-0002 core VB/VBA language execution objective
- **Depends on:** REQ-0140 and REQ-0141
- **Conflicts with:** None known

## Tailoring

This subset does not claim locale-aware or text-mode string comparison,
implicit conversion, Null or Error propagation, Variant truth rules, integer
bitwise logical operations, short-circuit operators, or control-flow statements.
Those behaviors require later requirements and reference evidence.

## Implementation Record

Implemented by the typed value and precedence parser in `src/evaluator.cpp`,
with unit and CLI coverage in the named CTest cases.

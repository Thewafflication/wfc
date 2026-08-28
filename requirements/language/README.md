# Core VB Language Requirements

**Content type:** Project requirement index

**Status:** Proposed

This requirement set defines source-language behavior implemented during
MP-0002. Each increment states its exact compatibility boundary so passing a
narrow corpus cannot be mistaken for complete VB6 language support.

## Requirements

- [REQ-0140 — Core expression execution subset](req-0140-core-expression-subset.md)
  defines the first typed expression grammar, values, operators, results, and
  stable failures accepted by the `Print` evaluator.
- [REQ-0141 — Module statement and variable subset](req-0141-module-statements-and-variables.md)
  defines sequential execution, typed module variables, assignments, comments,
  and multi-line output for the second MP-0002 increment.
- [REQ-0142 — Boolean and comparison expression subset](req-0142-boolean-and-comparison-expressions.md)
  defines Boolean values, comparisons, logical operators, precedence, and
  strict type failures for the third MP-0002 increment.
- [REQ-0143 — Single-line conditional statement subset](req-0143-single-line-conditional-statements.md)
  defines Boolean-gated `If ... Then ... Else` execution and branch diagnostics
  for the fourth MP-0002 increment.

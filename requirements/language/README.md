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
- [REQ-0144 — Block conditional statement subset](req-0144-block-conditional-statements.md)
  defines multiline `If ... Then` / `Else` / `End If` execution and structural
  diagnostics for the fifth MP-0002 increment.
- [REQ-0145 — Block ElseIf branch subset](req-0145-block-elseif-branches.md)
  defines ordered `ElseIf` selection, first-match execution, and branch
  diagnostics for the sixth MP-0002 increment.
- [REQ-0146 — While...Wend loop subset](req-0146-while-wend-loops.md)
  defines pre-test repetition, nested loop execution, and loop diagnostics for
  the seventh MP-0002 increment.
- [REQ-0147 — Pre-test Do loop subset](req-0147-pretest-do-loops.md)
  defines `Do While` and `Do Until` execution and diagnostics for the eighth
  MP-0002 increment.
- [REQ-0148 — Post-test Do loop subset](req-0148-posttest-do-loops.md)
  defines `Loop While` and `Loop Until` execution and diagnostics for the ninth
  MP-0002 increment.
- [REQ-0149 — Rem comment subset](req-0149-rem-comments.md) defines
  case-insensitive, statement-level `Rem` comments for the tenth MP-0002
  increment.
- [REQ-0150 — Exit Do subset](req-0150-exit-do.md) defines structured,
  innermost-loop termination for the eleventh MP-0002 increment.
- [REQ-0151 — For...Next loop subset](req-0151-for-next-loops.md) defines
  declared-`Long` counter iteration for the twelfth MP-0002 increment.
- [REQ-0152 — Exit For subset](req-0152-exit-for.md) defines structured,
  innermost counter-loop termination for the thirteenth MP-0002 increment.
- [REQ-0153 — Select Case equality subset](req-0153-select-case.md) defines
  ordered, same-type equality branches for the fourteenth MP-0002 increment.
- [REQ-0154 — Select Case value-list subset](req-0154-select-case-value-lists.md)
  defines comma-separated equality alternatives for the fifteenth MP-0002
  increment.
- [REQ-0155 — Select Case range subset](req-0155-select-case-ranges.md) defines
  inclusive numeric and string ranges for the sixteenth MP-0002 increment.
- [REQ-0156 — Select Case Is subset](req-0156-select-case-is.md) defines
  relational branch clauses for the seventeenth MP-0002 increment.
- [REQ-0157 — Typed constant subset](req-0157-typed-constants.md) defines
  immutable `Long`, `String`, and `Boolean` values for the eighteenth MP-0002
  increment.
- [REQ-0158 — Option Explicit subset](req-0158-option-explicit.md) defines
  strict module directive placement for the nineteenth MP-0002 increment.
- [REQ-0159 — Option Compare subset](req-0159-option-compare.md) defines binary
  and ASCII case-insensitive string comparison for the twentieth MP-0002
  increment.
- [REQ-0160 — Len function subset](req-0160-len-function.md) defines the first
  callable VBA string function for the twenty-first MP-0002 increment.
- [REQ-0161 — String case function subset](req-0161-string-case-functions.md)
  defines ASCII `LCase` and `UCase` calls for the twenty-second MP-0002
  increment.
- [REQ-0162 — String trim function subset](req-0162-string-trim-functions.md)
  defines `Trim`, `LTrim`, and `RTrim` calls for the twenty-third MP-0002
  increment.
- [REQ-0163 — String slice function subset](req-0163-string-slice-functions.md)
  defines `Left` and `Right` calls and multi-argument call parsing for the
  twenty-fourth MP-0002 increment.
- [REQ-0164 — Mid function subset](req-0164-mid-function.md) defines one-based
  substring extraction and optional call arguments for the twenty-fifth
  MP-0002 increment.
- [REQ-0165 — Character code function subset](req-0165-character-code-functions.md)
  defines ASCII `Asc` and `Chr` conversion for the twenty-sixth MP-0002
  increment.

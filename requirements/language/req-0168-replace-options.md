# REQ-0168 — Replace options subset

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0167 and REQ-0071

## Requirement

`Replace` shall accept its three required `String` arguments followed by up to
three positional `Long` arguments: `start`, `count`, and `compare`.

- `start` defaults to 1, is one-based, and must be positive. The returned value
  begins at this position; source characters before it are omitted.
- `count` defaults to -1, meaning replace every match. Zero replaces nothing;
  positive values limit the number of replacements; values below -1 fail.
- `compare` defaults to module-level `Option Compare`. Explicit
  `vbBinaryCompare` or `vbTextCompare` overrides the module mode.

A start beyond the expression returns an empty string. An empty search string
returns the selected suffix unchanged. Matching remains non-overlapping and
byte-oriented under the current string subset.

Named/omitted middle arguments, Variant coercion, and `vbDatabaseCompare`
execution remain outside this requirement.

## Diagnostics

Existing call, type, positive-start, and comparison-method diagnostics apply.
`WFC0082` reports a count below -1.

## Verification

- `tests/evaluator_tests.cpp` covers defaults, starts, bounded and zero counts,
  explicit compare overrides, empty/beyond-end results, and invalid arguments.
- `TC-MP0002-replace-options-cli` verifies all three optional arguments through
  `wfc --eval`.

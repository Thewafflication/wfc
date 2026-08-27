# REQ-0137 — Repeatable test and evidence dispatch

**Content type:** Project requirement

**Status:** Proposed

**Source:** MP-0001 test and evidence objective

## Requirement

WFC shall expose automated tests through CTest, assign stable test identifiers
and requirement/milestone labels, report failures with actionable diagnostics,
and retain sufficient structured context to reproduce each milestone gate,
including source revision, dependency revisions, toolchain, target, commands,
results, and artifact identities.

## Rationale

Passing output without its inputs and environment cannot support a durable
compatibility or release decision.

## Verification

**Method:** Inspection and test

Run the applicable CTest preset, inspect its registered tests and labels, and
validate a generated evidence record against the controlled evidence schema.

## Relationships

- **Derived from:** MP-0001 verification stage
- **Depends on:** REQ-0132 and REQ-0133
- **Conflicts with:** None known

## Tailoring

Local exploratory output may be transient, but no milestone gate may be closed
without the controlled evidence defined by the test strategy.

## Implementation Record

CTest dispatch, stable names, labels, and failure output exist for the vertical
slice. Structured evidence generation, validation, and CI retention remain.

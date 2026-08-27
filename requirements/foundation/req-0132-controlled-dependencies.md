# REQ-0132 — Controlled WSP and WCRT dependencies

**Content type:** Project requirement

**Status:** Proposed

**Source:** MP-0001 dependency and process baseline objective

## Requirement

WFC shall identify WSP and WCRT as root Git submodules, pin each gitlink to an
immutable reviewed release commit, record its version, license, applicability,
integration boundary, and upgrade procedure, and fail review when a recorded
baseline and gitlink disagree.

## Rationale

The product process and runtime foundation cannot be reproducible when their
identity or adoption decisions depend on an unrecorded local checkout.

## Verification

**Method:** Inspection and test

Compare `.gitmodules`, both gitlinks, the WSP adoption record, and the WCRT ADR.
An automated dependency check shall resolve each gitlink and require the exact
approved commit before MP-0001 closes.

## Relationships

- **Derived from:** MP-0001 baseline stage
- **Depends on:** None
- **Conflicts with:** None known

## Tailoring

A dependency upgrade requires a reviewed impact assessment and corresponding
record update; floating branches are not permitted for a controlled baseline.

## Implementation Record

WSP 1.1.0 and WCRT 1.1.1 are pinned and documented. Automated agreement
checking remains to be implemented.

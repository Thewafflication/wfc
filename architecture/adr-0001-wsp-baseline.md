# ADR-0001: Adopt WSP 1.1.0 by Root Submodule

**Content type:** Architecture decision record

**Status:** Accepted

**Date:** 2026-08-27

## Context

WFC needs a controlled engineering-process baseline. The repository already
contained WSP at a commit four changes past release 1.1.0, while WSP directs
adopting projects to pin an immutable controlled release.

## Decision Drivers

- Exact and reviewable process inputs
- Repeatable developer and CI checkout
- Project-owned tailoring without modifying shared WSP content
- Alignment with WSP's own adoption guidance

## Considered Options

1. Pin WSP release 1.1.0 as a root Git submodule.
2. Continue following WSP `master` at an arbitrary commit.
3. Copy WSP templates and tools into WFC.

## Decision

WFC adopts WSP release 1.1.0 at commit
`8c2adb4afb9f95a5632ec783e37a79c29b1f90f5` through the root `wsp/`
submodule. WSP content is read-only from WFC. Project requirements, plans,
tailoring, evidence, and generated outputs remain outside the submodule.

## Rationale

The release pin provides immutable process inputs without duplicating shared
content. Following an unreleased branch would make project obligations change
without a WFC review; copying would lose upstream identity and upgrade history.

## Consequences

### Positive

- Every WFC revision identifies its exact process baseline.
- WSP upgrades are visible as one gitlink change plus an adoption impact review.

### Negative

- Clones must initialize submodules.
- Fixes to WSP must be proposed upstream before WFC adopts them.

### Follow-up

- Keep `planning/wsp-adoption-record.md` synchronized with the gitlink.
- Initialize `wsp` and `wcrt` in local setup and CI.

## References

- `planning/wsp-adoption-record.md`
- `wsp/architecture/adr-0001-adoption-by-git-submodule.md`
- `planning/MP-0001-foundation-and-feasibility.md`

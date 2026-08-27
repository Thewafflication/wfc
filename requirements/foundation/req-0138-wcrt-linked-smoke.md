# REQ-0138 — WCRT-linked target proof

**Content type:** Project requirement

**Status:** Proposed

**Source:** MP-0001 WCRT feasibility gate

## Requirement

WFC shall provide a fail-closed CMake integration for the exact approved WCRT
package, select architecture-matching headers and libraries, and build and run
a minimal WFC-owned consumer that proves the intended WCRT service and ABI on
Windows x86, x64, and ARM64. Configuration shall fail with a useful diagnostic
when the package identity, architecture, or required artifacts do not match.

## Rationale

A source gitlink and ADR establish dependency intent but do not prove that WFC
can consume the released runtime artifacts on its target matrix.

## Verification

**Method:** Test

Exercise successful package discovery, rejected missing/mismatched packages,
link inspection, and native smoke execution for each required architecture.
Retain the WCRT package identity and artifact hashes.

## Relationships

- **Derived from:** MP-0001 dependency and build stages
- **Depends on:** REQ-0132 and REQ-0133
- **Conflicts with:** None known

## Tailoring

Cross-compilation may establish only configure, compile, and link evidence.
Native ARM64 execution may occur on a separate controlled runner, but it remains
a required MP-0001 gate.

## Implementation Record

Not implemented. ADR-0002 fixes the package and integration boundary.

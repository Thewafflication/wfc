# REQ-0133 — Reproducible native build matrix

**Content type:** Project requirement

**Status:** Proposed

**Source:** MP-0001 build and target-matrix objectives

## Requirement

WFC shall provide a top-level CMake build using C++20 with named configure and
build presets for Windows x86, x64, and ARM64. A clean checkout with documented
prerequisites shall place generated files beneath `out/` and build every
MP-0001 production and test target without editing tracked files.

## Rationale

All three product architectures must enter the design and build graph before
later ABI and platform assumptions become expensive to reverse.

## Verification

**Method:** Test

Configure and build every named preset from a clean workspace. Retain compiler,
SDK, CMake, architecture, command result, and artifact identity in milestone
evidence. Native execution is separately required for compatibility claims.

## Relationships

- **Derived from:** MP-0001 build stage
- **Depends on:** REQ-0132
- **Conflicts with:** None known

## Tailoring

A target may be temporarily cross-compiled without execution, but compilation
alone shall not be reported as target execution or compatibility evidence.

## Implementation Record

CMake and debug presets compile successfully for x86, x64, and ARM64 on the
development host. GitHub Actions configures and builds every preset from a
checkout with controlled submodules; x86 and x64 use Windows x64 runners and
ARM64 uses a native Windows ARM64 runner. Each job retains structured context
and artifact identities. GitHub Actions
[run 33142252890](https://github.com/Thewafflication/wfc/actions/runs/33142252890)
configured, built, tested, and retained evidence successfully for all three
architectures on 2026-08-28 UTC.

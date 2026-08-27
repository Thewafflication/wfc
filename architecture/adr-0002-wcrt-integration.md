# ADR-0002: Integrate WCRT as a Pinned External Runtime

**Content type:** Architecture decision record

**Status:** Accepted

**Date:** 2026-08-27

## Context

WFC targets Windows x86, x64, and ARM64 and is required to use WCRT as its C
runtime foundation. WCRT has its own TinyCC build, tests, packaging, release
process, and GPL-3.0-or-later license. Its sources are not a native CMake
subproject and should not be silently reconfigured by WFC.

## Decision Drivers

- Immutable, reviewable dependency identity
- Reuse of WCRT's tested architecture-specific artifacts
- Separation of dependency and product build ownership
- Reproducible local/CI provisioning
- Explicit licensing and ABI boundaries

## Considered Options

1. Pin WCRT source as a submodule and consume an exact WPM/release build.
2. Add WCRT sources directly to WFC CMake targets.
3. Use the host Microsoft/UCRT runtime without WCRT.

## Decision

WFC pins WCRT release 1.1.1 at commit
`27b42dea18d27092df26cd0baed2c8c6f6939da4` in the root `wcrt/`
submodule. Production integration shall consume the matching WCRT release/WPM
artifacts through an explicit `WCRT_ROOT` or installed `WCRT_HOME`, selecting
the target-specific include and library paths. WFC shall not modify WCRT sources
or build them implicitly as part of ordinary CMake configuration.

The initial front-end feasibility slice has no C runtime service dependency and
therefore does not link WCRT. The first component that allocates, performs I/O,
or owns process startup through the platform layer shall activate and verify
the imported WCRT target. This is a required MP-0001 exit gate, not an optional
product dependency.

## Rationale

WCRT's release artifacts are the units tested by its project. Directly adding
sources to an unrelated CMake graph would create a new, unqualified build
configuration. Omitting WCRT would violate the product foundation.

## Consequences

### Positive

- Dependency source, license, release, and binary inputs are explicit.
- WCRT can evolve independently and WFC upgrades require review.
- Target selection remains visible in configuration and evidence.

### Negative

- Developers and CI must provision a matching WCRT package.
- C++ ABI and standard-library facilities remain compiler-owned; WCRT supplies
  the C/platform runtime boundary, not a replacement C++ standard library.
- WFC distribution must adopt a GPL-3.0-compatible licensing posture and meet
  the dependency's redistribution obligations before public release.

### Follow-up

- Add a fail-closed CMake imported-target helper for `WCRT_ROOT`/`WCRT_HOME`.
- Build and run one WCRT-linked smoke consumer on each required architecture.
- Record the WFC license and third-party notices before a distributed baseline.

## References

- `wcrt/README.md`
- `wcrt/LICENSE.txt`
- `planning/MP-0001-foundation-and-feasibility.md`
- `planning/compatibility-profile-1.0.md`

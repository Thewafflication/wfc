# ADR-0003: CMake, CTest, C++20, and Target Matrix

**Content type:** Architecture decision record

**Status:** Accepted

**Date:** 2026-08-27

## Context

WFC needs a repeatable C++ build for Windows x86, x64, and ARM64, deterministic
unit/integration dispatch, strict diagnostics, and room for architecture-specific
COM and runtime ABI tests.

## Decision Drivers

- Supported Windows architecture matrix
- Compatibility with current Visual Studio and automation environments
- Target-scoped warnings and dependencies
- Repeatable local and CI test commands
- No generated files in the source tree

## Considered Options

1. CMake presets with CTest and C++20.
2. Visual Studio solution/project files as the authority.
3. A custom PowerShell/TinyCC-only build.

## Decision

CMake is the authoritative WFC build description, CTest is the automated test
dispatcher, and CMake presets define Windows x86, x64, and ARM64 Debug/Release
configurations. Project-owned C++ uses C++20 without compiler extensions.

MSVC is the initial C++ compiler. Owned targets compile at `/W4 /WX` with
standards-conformance switches. Compiler-specific policies remain target scoped.
TinyCC remains relevant to WCRT and selected ABI probes, but is not the WFC C++
compiler. Build output belongs under `out/`.

## Rationale

CMake provides one reviewable model for multiple architectures and generators;
CTest provides consistent selection and reporting. Checked-in IDE files would
duplicate configuration, while TinyCC is a C compiler and cannot own the C++20
front end.

## Consequences

### Positive

- Developers and CI share configure, build, and test entry points.
- Strict diagnostics prevent warning debt in the initial baseline.
- Architecture-specific tests can be selected without source duplication.

### Negative

- Visual Studio Build Tools are required for ordinary WFC builds.
- Cross-compiled ARM64 evidence does not substitute for native ARM64 execution.

### Follow-up

- Add native CI execution for each architecture by MP-0008.
- Add formatting, static analysis, sanitization where supported, resource, and
  release-hardening gates during MP-0001.

## References

- `CMakeLists.txt`
- `CMakePresets.json`
- `planning/MP-0001-foundation-and-feasibility.md`

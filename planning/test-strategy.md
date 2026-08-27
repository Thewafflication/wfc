# MP-0001 Test and Evidence Strategy

**Content type:** Project test strategy

**Status:** Proposed

## Objectives

- Verify requirements at the lowest deterministic layer that can observe them.
- Separate Microsoft reference observations from WFC conformance results.
- Use one repeatable dispatcher for owned automated tests.
- Preserve enough environment, input, output, and binary identity to reproduce
  failures and compatibility claims.

## Test Layers

| Layer | Scope | Dispatcher or evidence |
| --- | --- | --- |
| Unit | Lexer, parser, diagnostics, semantic values, IR, pure VBA behavior | CTest labels `unit` and requirement/test identifiers |
| Integration | Project formats, runtime/platform, COM, forms, controls, persistence | CTest labels `integration`, `windows`, and architecture |
| Reference | Small VB6 programs recording observed Microsoft behavior | `tests/reference/` source plus retained structured output |
| Conformance | Same scenario against WFC and approved reference expectation | Requirement-linked CTest cases and comparison output |
| Release | Complete matrix, artifacts, evidence, docs, integrity, install/rollback | Release-readiness record and CI artifacts |

## Initial Dispatcher

CMake configures owned C++ tests and `ctest --preset windows-x64-debug` executes
the initial suite. Each test has a stable name and labels. Failed commands
return nonzero and print the scenario and difference; tests do not convert
execution errors into passes.

## Evidence Boundary

Generated output belongs under `out/` or a later controlled evidence directory.
Microsoft binaries and proprietary help are never copied into release artifacts.
Reference evidence records source, component hashes/versions, architecture,
locale and relevant OS state. Visual tests retain exact images plus comparison
settings when pixels are part of the claim.

## MP-0001 Acceptance

The vertical slice requires unit tests for accepted and rejected syntax plus an
end-to-end CLI demonstration from source text to observable output. A later
MP-0001 increment adds CI, traceability validation, structured evidence, and a
WCRT-linked target smoke test before the milestone closes.

## References

- `wsp/testing/test-strategy.md`
- `planning/MP-0001-foundation-and-feasibility.md`
- `architecture/README.md`

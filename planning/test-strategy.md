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

## Automated Dispatcher

CMake configures owned C++ tests and the matching CTest preset executes each
architecture suite. GitHub Actions runs x86 and x64 on Windows x64 and ARM64 on
a native Windows ARM64 runner. Each test has a stable name and labels. Failed
commands return nonzero and print the scenario and difference; tests do not
convert execution errors into passes.

Every CI matrix job retains JUnit results, runner and dependency context,
SHA-256 identities for tested Debug binaries and symbols, and CTest failure
output where available. Artifact names identify the source revision, target
architecture, and Debug configuration.

## Evidence Boundary

Generated output belongs under `out/` or a later controlled evidence directory.
Microsoft binaries and proprietary help are never copied into release artifacts.
Reference evidence records source, component hashes/versions, architecture,
locale and relevant OS state. Visual tests retain exact images plus comparison
settings when pixels are part of the claim.

## MP-0001 Acceptance

The vertical slice requires unit tests for accepted and rejected syntax plus an
end-to-end CLI demonstration from source text to observable output. CI and its
initial structured Debug evidence are implemented. Traceability validation and
a WCRT-linked target smoke test remain before the milestone closes.

## References

- `wsp/testing/test-strategy.md`
- `planning/MP-0001-foundation-and-feasibility.md`
- `architecture/README.md`

# ADR-0007: Native Packaging and the VBRUN Compatibility Library

**Content type:** Architecture decision record

**Status:** Proposed (implementation started on the owner's instruction, 2026-10-12)

**Date:** 2026-10-12

## Context

MP-0007 must produce deployable native programs on x86, x64 and ARM64 and offer
an x86 `MSVBVM60.DLL`-compatible export surface (`REQ-0099`–`REQ-0105`). WFC is
a text-reparsing interpreter. A machine-code back end needs an intermediate
representation, a calling convention per target and a native runtime; none of
that exists, and the VBRUN export signatures are not encoded in the PE export
table (they need compiled probes). A complete code generator is therefore not one
increment's work and cannot be verified without reference binaries.

## Decision

1. **Packaged programs first.** `wfc --build <project.vbp> -o <app.exe>` writes a
   self-contained executable: a copy of the running `wfc` image (so the output
   has the architecture of the build, x86, x64 or ARM64), followed by a payload
   holding every file of the project directory and a fixed trailer. At start-up
   `wfc` checks its own image for the trailer; when present it extracts the
   payload into a per-payload directory below the user's temporary directory
   (reused when it already exists) and runs the project from there with the
   command line passed through. Output is the same as running the project with
   `wfc`. Native machine-code generation stays behind this interface: `--build`
   can later emit a compiled image without changing how users build programs.
2. **VBRUN library.** `wfcvbrun.dll` (CMake target `wfc_vbrun`) is built for the
   architecture of the preset. Its export directory is generated from
   `evidence/reference/msvbvm60-6.0-runtime.json` into a module-definition file:
   ordinal base 100, 1,911 slots, the 635 named exports at their reference
   ordinals, holes left empty. Every export is a stub that records the ordinal
   and reports failure through one entry point, so a program that reaches an
   unimplemented helper stops with a diagnostic naming it instead of crashing.
   Helpers receive real implementations only when their signature is confirmed by
   a probe or a documented prototype.
3. **Architecture bridge.** The library is an x86 ABI. On x64 and ARM64 a native
   x86 program is run in an x86 helper process, never loaded in-process; the
   helper process and its bridge are deferred until x86 helper binaries can be
   exercised there.

## Consequences

- Deployment works on all three architectures now, with the runtime embedded.
- No claim of machine-code generation is made; the roadmap gate for it is
  recorded as deferred in the MP-0007 plan.
- The export layout can be verified on any machine by parsing the built DLL.

## Alternatives considered

- **Compile VB6 to C++ and invoke a compiler.** Needs a toolchain on the user's
  machine; rejected for deployment.
- **Emit machine code directly for one target.** Unverifiable without reference
  binaries and the signature probes; deferred.

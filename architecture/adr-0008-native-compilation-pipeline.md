# ADR-0008: Native Compilation Pipeline

**Content type:** Architecture decision record

**Status:** Proposed (implementation started on the owner's instruction, 2026-10-20)

**Date:** 2026-10-20

## Context

WFC today is a text-reparsing interpreter: `wfc::detail::Interpreter` reads
source text and executes while it parses. There is no syntax tree, no type
information that survives a statement, and no representation a code generator
could consume. MP-0009 must produce machine code native to x86, AMD64 and ARM64
(`ADR-0007`, owner decision of 2026-10-12: no translation layer, no emulation, no
x86 helper process), and the interpreter remains the behavioral reference until
native output passes the corpus.

## Decision

1. **A separate compiler front end.** A new library `wfc_compiler`
   (`src/compiler/`) lexes and parses VB source into an abstract syntax tree
   (AST), resolves names and types (semantic analysis), and lowers the result to
   a small intermediate representation (IR). The interpreter is not changed to
   serve it; the two share only the runtime services and the test corpus, so the
   interpreter stays a stable reference.
2. **IR.** A typed, three-address IR over virtual registers and basic blocks,
   with scalar types `Boolean`, `Integer` (16 bit), `Long` (32 bit), `Single`,
   `Double`, `Currency` and pointer-sized handles for `String`, `Variant`,
   arrays and objects. Back ends map IR to machine code with a simple linear
   register allocator; optimization is out of scope until the corpus passes.
3. **One back end per architecture, one shared runtime.** AMD64 comes first
   (the development machine), then x86 and ARM64 on the same IR. Each back end
   emits its own encoding and follows the platform calling convention of its
   target (Windows x64, x86 `__stdcall`, AAPCS64), so generated code calls the
   C++ runtime directly.
4. **The runtime is the existing C++ code, reached through an entry table.**
   Strings, Variants, arrays, objects, errors, the form and control host and the
   `Declare` machinery stay in C++. Generated code calls them through a
   versioned table of function pointers (`NativeRuntimeApi`) that the loader
   fills in at start-up; the table is the contract between compiler output and
   runtime, and a change to it changes its version number.
5. **Output is a native payload in the packaged program.** `wfc --build`
   keeps writing a self-contained executable: the runtime image plus a payload.
   For native builds the payload carries the generated code of the target
   architecture and its relocations instead of the project text; at start-up the
   runtime maps the code executable, patches it, fills the entry table and
   jumps to the program's entry. No linker or system toolchain is needed on the
   user's machine, and the same mechanism serves all three architectures.
   Replacing the runtime image by a smaller runtime-only image is a later size
   optimization, not part of this decision.
6. **The interpreter is the oracle.** Every native test runs the same program
   through the interpreter and through the native path and compares output. A
   language feature is "compiled" only when its native tests pass; a program
   that uses a feature the compiler does not support yet is rejected with a
   diagnostic naming the construct. It does not fall back silently to the
   interpreter (no hidden second execution path).
7. **A development entry point.** `wfc --native <project.vbp>` compiles in memory
   and runs the result, for the test suite. It reports "not available on this
   architecture" (exit code 77) where no back end exists yet; those tests are
   skipped there.

## Consequences

- Two implementations of VB semantics exist during MP-0009. The oracle tests and
  the shared runtime keep them in step; the compiler supports a growing subset
  and says plainly what it does not.
- Shipping needs no compiler or linker on the user's machine.
- The x86 and ARM64 back ends cannot be run locally; they are exercised by
  GitHub Actions only (`AGENTS.md`).
- The entry table fixes the boundary early, so runtime work and back-end work
  can proceed separately.

## Alternatives considered

- **Compile to C++ and call a system compiler.** Needs a toolchain on the user's
  machine; rejected in `ADR-0007`.
- **Generate a complete PE image with our own linker and a separate runtime
  DLL.** The cleanest result, but it adds an import/relocation writer for three
  architectures before any code runs; deferred as a size and layout optimization
  on top of decision 5.
- **Translate the interpreter's execution into closures or bytecode.** Faster
  than text reparsing but not machine code, so it does not meet the milestone.
- **Retrofit the interpreter with an AST.** Entangles the reference with the new
  work and risks regressions in 400 passing tests.

# WFC Architecture

**Document ID:** WFC-ARCH-0001

**Status:** Proposed for MP-0001

## Goals and Boundaries

WFC accepts VB6 source/projects and selected legacy x86 binaries, produces or
executes a target-independent semantic representation, and supplies the runtime,
COM/OLE, forms, intrinsic controls, and common controls required by the approved
1.0 compatibility profile. The VB6 IDE, JetSQL, and other removed features are
outside this architecture except for stable unsupported-feature diagnostics.

## Component Model

```text
VB source/project          legacy x86 binary
        |                         |
        v                         v
 source/project loader      VBRUN ABI adapter
        |
        v
 lexer -> parser -> binder/type system
                    |
                    v
             semantic IR
              |       |
              v       v
        interpreter   native code generator
              \       /
               v     v
             VB runtime services
              |      |       |
              v      v       v
          COM/OLE  forms/UI  VBA library
              \      |       /
               v     v      v
              platform boundary
                      |
                      v
                 Win32 + WCRT
```

## Components and Responsibilities

| Component | Responsibility | Dependency direction | Initial milestone |
| --- | --- | --- | --- |
| Source/project | Decode and validate VB project and source formats; preserve locations and resources | None beyond diagnostics and filesystem abstraction | MP-0001/MP-0002 |
| Lexer/parser | Produce immutable syntax with stable diagnostics and no side effects | Source only | MP-0001/MP-0002 |
| Binder/type system | Resolve names, declarations, types, overload/default-member behavior, and semantic errors | Syntax and metadata interfaces | MP-0002/MP-0003 |
| Semantic IR | Target-independent executable meaning shared by interpreter and code generator | Bound semantic values | MP-0001 onward |
| Interpreter | Deterministic reference execution and development path | IR plus abstract runtime services | MP-0001/MP-0002 |
| Native code generator | Produce x86/x64/ARM64 executable code and runtime calls | IR and target ABI descriptions | MP-0007 |
| VB runtime | Values, strings, arrays, variants, objects, errors, module state, helper ABI, and lifecycle | WCRT/platform and COM abstractions | MP-0002 onward |
| VBA library | Standard classes and language-service functions | VB runtime and platform interfaces | MP-0002 onward |
| COM/OLE | Automation, type information, events, activation, persistence, and object hosting | VB runtime and Win32 COM | MP-0003/MP-0005 |
| Forms/UI | Forms, containment, menus, intrinsic controls, input, drawing, accessibility, and persistence | Runtime, COM, and abstract platform/UI services | MP-0004 |
| MSComctl | Nine common-control-compatible Automation classes | Forms/UI, COM, and image/persistence services | MP-0006 |
| Platform | Files, clocks, locale, process startup, allocation, Win32 handles, and target facilities | WCRT and Win32 only | MP-0001 onward |
| CLI/tools | Compile, run, inspect, probe, migrate, and report without owning semantics | Public compiler/runtime interfaces | MP-0001 onward |

Dependencies point from product shells toward semantic cores and abstract
runtime interfaces. Parser, binder, and IR code shall not call Win32, COM, UI,
filesystem, process, registry, environment, or wall-clock APIs directly.

## State and Ownership

- A compilation owns source buffers, syntax, symbols, diagnostics, and semantic
  IR; immutable data may be shared by explicit lifetime ownership.
- A runtime context owns module state, objects, errors, locale, open resources,
  platform services, and cancellation. Process-global mutable language state is
  prohibited unless VB6 requires it and the requirement explicitly defines it.
- COM reference counts and Windows handles have one documented owner. Transfers
  and borrowed references are explicit; cleanup is idempotent.
- No user program executes until its complete compilation unit has passed the
  required parse and binding gates.

## Compatibility and Target Matrix

| Target | Source/project compatibility | WFC native output | Unmodified VB6 binary profile | Required execution evidence |
| --- | --- | --- | --- | --- |
| Windows x86 | Required | Required | Selected 1.0 profile | Native x86 |
| Windows x64 | Required | Required | x86 isolated out of process only | Native x64 |
| Windows ARM64 | Required | Required | x86 isolated/emulated out of process only | Native ARM64 |

The reference VB6 installation is x86. Cross-architecture results shall match
source-visible semantics unless an approved requirement records an ABI- or
platform-mandated variance.

## Verification Strategy

- Lexer, parser, types, values, IR, and most VBA functions use deterministic
  unit tests without Win32 or UI state.
- VB6 probes establish defaults, event order, errors, pixels, persistence,
  calling conventions, and undocumented behavior.
- COM, forms, controls, filesystem, process, and platform services use Windows
  integration tests with controlled fixtures.
- Reference and WFC runners emit comparable structured evidence.
- Every architecture claim requires native execution of the final relevant
  artifact; compilation alone is not reported as execution success.

## Durable Decisions

| Decision | Record |
| --- | --- |
| WSP release pin and adoption | [ADR-0001](adr-0001-wsp-baseline.md) |
| WCRT dependency and integration boundary | [ADR-0002](adr-0002-wcrt-integration.md) |
| CMake, CTest, C++ edition, and target matrix | [ADR-0003](adr-0003-cmake-toolchain.md) |

## Open MP-0001 Decisions

- Exact semantic IR representation and versioning
- Diagnostic schema and source encoding policy
- Interpreter versus compiler ownership of late-bound operations
- Public embedding/compiler APIs and ABI stability point
- WCRT imported-target/package verification details
- Oldest supported Windows version for WFC, distinct from WCRT's range

These decisions shall be resolved or explicitly deferred before architecture
acceptance for the 0.1.0 baseline.

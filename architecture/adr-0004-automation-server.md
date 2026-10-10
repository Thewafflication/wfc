# ADR-0004: Automation Server Hosting

**Content type:** Architecture decision record

**Status:** Accepted

**Date:** 2026-10-10

## Context

MP-0003 delivers the Automation client: `CreateObject`, `GetObject`,
type-library references, early binding, and event sinks all drive real COM
servers through `IDispatch` (`REQ-0286`, `REQ-0288`). The milestone plan also
asks how a WFC program exposes its own public classes (an ActiveX EXE or DLL
project) to other COM clients, because that choice shapes the interpreter's
threading model, the class metadata it must keep, and the registry footprint of
the test suite.

The interpreter is single-threaded, keeps all state in one `Interpreter`
object, and runs on one large-stack thread inside a single-threaded apartment.
It can already describe every public class member (`ClassDef`), instantiate
classes, and raise events.

## Decision Drivers

- No registry writes or system changes during automated tests.
- One interpreter thread: a server must not run VB code concurrently.
- Reuse of `ClassDef` for dispatch rather than a second class model.
- Native compilation (MP-0007) will change how in-process servers are built.
- Component authoring (registration, licensing, property bags) is MP-0005.

## Considered Options

1. **No server in MP-0003** and design the mechanism with component authoring.
2. **Out-of-process local server hosted by `wfc.exe`:** `wfc --server
   project.vbp` registers a class factory per public creatable class with
   `CoRegisterClassObject`; calls arrive on the interpreter's apartment and
   run as ordinary member calls through a dynamic `IDispatch` built from
   `ClassDef`. `wfc --register` / `--unregister` write the per-user
   `LocalServer32`/`ProgID` keys explicitly.
3. **In-process DLL server** compiled from the project: needs the native
   compiler and a runtime image (MP-0007).

## Decision

Take option 2 as the design of record, and **defer its implementation to
MP-0005**, where component authoring, registration, licensing and type-library
generation are already scheduled. MP-0003 ships no server code. Option 3
remains the long-term in-process path once MP-0007 exists.

The client-side pieces MP-0003 does build are the reusable half of this design:
the `VARIANT` marshalling (`value_to_variant`, `variant_to_value`), the
identity-preserving wrapper registry, and the dispatch-id and type-information
helpers.

## Consequences

- Programs can consume COM today; they cannot yet be consumed as COM objects.
- No test in MP-0003 writes the registry. A server test will use an in-process
  `CoRegisterClassObject` with no `LocalServer32` entry.
- The server will need the interpreter to accept calls re-entrantly from the
  message loop; `DoEvents` already pumps messages, and the COM event path
  already runs handlers re-entrantly (`deliver_com_event`).
- `ClassDef` will need a type-information export (`ITypeInfo` synthesized from
  members) for early-bound clients; this is part of MP-0005 type-library work.

## Rationale

An out-of-process server matches the single-threaded interpreter and avoids
loading the interpreter into foreign processes. Deferring keeps MP-0003 focused
on the client semantics that existing VB programs depend on most, and keeps the
registry work with the milestone that owns registration.

## References

- [MP-0003 plan](../planning/MP-0003-classes-projects-automation.md)
- [REQ-0286](../requirements/language/req-0286-com-automation-client.md)
- [REQ-0288](../requirements/language/req-0288-type-library-references.md)

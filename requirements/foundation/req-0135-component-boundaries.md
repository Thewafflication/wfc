# REQ-0135 — Component and dependency boundaries

**Content type:** Project requirement

**Status:** Proposed

**Source:** MP-0001 architecture objective

## Requirement

WFC shall document ownership and dependency direction for source/project
loading, parsing, binding and types, semantic IR, interpretation, native code
generation, VB runtime services, VBA, COM/OLE, forms and controls, MSComctl,
platform services, and product tools. Parser, binder, and IR components shall
not directly depend on Win32, COM, UI, filesystem, registry, environment,
process, or wall-clock APIs.

## Rationale

Portable semantic behavior and deterministic testing require platform effects
to enter through explicit runtime and platform boundaries.

## Verification

**Method:** Inspection and test

Review the architecture record and target dependency graph. Add an automated
dependency-boundary check when the corresponding source components are
introduced.

## Relationships

- **Derived from:** MP-0001 architecture stage
- **Depends on:** REQ-0132
- **Conflicts with:** None known

## Tailoring

Any direct platform dependency outside the platform/runtime layer requires an
accepted ADR describing why interface injection cannot preserve the behavior.

## Implementation Record

The initial component model, ownership rules, target matrix, and open decisions
are recorded in `architecture/README.md`. Automated enforcement is pending the
creation of the corresponding targets.

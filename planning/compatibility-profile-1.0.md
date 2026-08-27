# WFC 1.0 Compatibility Profile

**Content type:** Product scope and compatibility record
**Status:** Proposed
**Target release:** 1.0.0
**Owner:** Project maintainer
**Approval:** Pending roadmap review

## Product Claim

WFC 1.0.0 is intended to be a mostly complete, evidence-backed replacement for
the Visual Basic 6 language toolchain and application runtime on supported
Windows targets. It is a source, project, Automation, UI, and selected native
binary compatibility product; it is not a recreation of the VB6 IDE.

“Mostly complete” does not mean an informal percentage. For release, every
identified feature shall have exactly one controlled disposition:

- **Required and verified** — implemented and accepted against the recorded
  VB6 reference baseline;
- **Required with approved variance** — implemented with a documented,
  reviewed difference that preserves the supported use case;
- **Removed from the profile** — intentionally absent because the feature is
  obsolete, unsafe, dependent on an abandoned platform, or outside the product
  purpose; or
- **Deferred** — not claimed by 1.0.0 and assigned to a named later release or
  completion condition.

Unimplemented or untested behavior without one of these dispositions is a
release blocker. Removed and deferred features are not counted as compatible
and shall be identified in user-facing compatibility documentation.

## Required 1.0 Scope

### Language and Project Compatibility

- Parse, bind, compile, and execute the supported VB6 language, including
  modules, procedures, expressions, statements, intrinsic and user-defined
  types, arrays, variants, objects, classes, events, properties, error handling,
  conditional compilation, and control arrays.
- Load the ordinary VB6 project and source artifacts needed by supported
  applications, including `.vbp`, `.bas`, `.cls`, `.frm/.frx`, `.ctl/.ctx`,
  and `.pag` files where the corresponding component remains in scope.
- Preserve VB6 defaults, conversions, evaluation rules, errors, lifecycle, and
  observable event order for the accepted compatibility corpus.
- Produce runnable WFC applications for Windows x86, x64, and ARM64 using the
  architecture policy established by the project.

### VBA and Runtime Services

- Provide the in-scope VBA 6 standard-library classes, functions, constants,
  locale behavior, errors, filesystem operations, date/time functions,
  formatting, financial functions, interaction functions, and collections.
- Provide the WFC compiler/runtime ABI required by WFC-produced programs.
- On x86, provide the approved `MSVBVM60.DLL` compatibility profile for legacy
  native VB6 programs, including required export names, ordinals, calling
  conventions, value layouts, errors, and cleanup behavior.
- Isolate x86-only legacy binary execution from native x64 and ARM64 processes;
  x64 and ARM64 support is source/project compatibility, not in-process loading
  of unmodified x86 binaries.

### Forms and Controls

- Implement ordinary forms, MDI forms, menus, control containment, control
  arrays, lifecycle, modality, focus, validation, drawing, persistence, input,
  accessibility, and supported drag-and-drop behavior.
- Implement the intrinsic visual controls retained by the 1.0 profile.
- Implement the nine documented `MSComctlLib` controls and their required
  collections, events, persistence, images, keyboard behavior, and rendering.
- Provide reference-backed behavior on supported Windows versions while
  documenting intentional visual differences caused by current OS themes,
  accessibility settings, DPI, or architecture.

### COM, OLE, and Component Authoring

- Implement the required `stdole` font and picture types, interfaces,
  persistence functions, and constants.
- Support Automation invocation, connection-point events, COM class identity,
  registration/activation, reference counting, type information, and supported
  ActiveX DLL/EXE/control scenarios.
- Support `UserControl` and its property persistence and event model when used
  by the accepted component corpus.
- Support modern OLE2/COM object hosting required by accepted applications.

### Engineering and Release Quality

- Build reproducibly with the selected WCRT and WSP baselines.
- Pass required unit, integration, reference-conformance, accessibility,
  architecture, static-analysis, packaging, and release-integrity gates.
- Ship compatibility documentation that states supported files, language
  features, APIs, architectures, Windows versions, exclusions, variances, and
  migration guidance.

## Approved Removal

| Feature | Disposition | 1.0 behavior | Rationale | Approval basis |
| --- | --- | --- | --- | --- |
| `___MSJetSQLHelp` interface and Jet SQL help/parser integration | Removed from 1.0 profile | No interface implementation or Jet SQL compatibility claim; encountering a dependency shall produce a documented unsupported-feature diagnostic | Hidden legacy Jet-specific assistance is not required for the modern VB6 application/runtime replacement | Maintainer direction, 2026-08-27 |

The removal covers the hidden 119-member `___MSJetSQLHelp` interface discovered
in `VB6.OLB`. It does not by itself remove all database access, generic data
binding, or the intrinsic `Data` control; those require separate dispositions.

## Existing Product Exclusions

These areas are outside the 1.0 product claim unless a later approved change
brings them into scope:

- the VB6 IDE shell, visual designer, source editor, debugger, wizards, setup
  toolkit, source-control integration, and `VBIDE` extensibility model;
- redistribution of proprietary Microsoft binaries, documentation, icons, or
  other copyrighted product assets;
- source or binary support for non-Windows operating systems; and
- in-process execution of x86 VB6 binaries inside x64 or ARM64 processes.

WFC may provide its own command-line tools and form/resource compiler. Excluding
the VB6 IDE does not exclude loading or building ordinary VB6 project files.

## Legacy Features Requiring Explicit Disposition

The following features appear obsolete or unusually costly but are not removed
by the JetSQL decision. MP-0002 shall decide each before its affected subsystem
becomes a release gate:

| Feature | Current disposition | Decision question |
| --- | --- | --- |
| DDE links and `Link*` members | Proposed; not yet accepted | Remove, provide a disabled compatibility stub, or implement an optional legacy profile? |
| Intrinsic `Data` control and DAO/Jet database behavior | Proposed; not yet accepted | Retain generic binding only, implement DAO compatibility, or remove the control? |
| OLE1 conversion and `SaveToOle1File` | Proposed; not yet accepted | Remove only OLE1 conversion while retaining OLE2 hosting? |
| `UserDocument`/ActiveX Documents | Proposed; not yet accepted | Required by the accepted application corpus or removable legacy host? |
| `PropertyPage` hosting | Proposed; not yet accepted | Required for component authoring or replaceable with programmatic configuration? |
| Legacy WinHelp and context-help integration | Proposed; not yet accepted | Preserve IDs/API behavior, bridge to current help, or report unsupported UI? |

A candidate is not silently omitted. Until approved otherwise, its existing
requirement remains Proposed and its milestone shall include either
implementation evidence or a controlled removal/deferment record.

## Compatibility Acceptance

The 1.0.0 claim requires:

1. a closed inventory mapping every discovered API, language feature, file
   format, runtime service, and behavior to an applicable requirement or an
   explicit removal/deferment;
2. passing WFC tests for every applicable requirement on the required target
   matrix;
3. retained VB6 reference evidence for behavior not fully specified by public
   interfaces or documentation;
4. representative real-project builds and executions covering forms, COM,
   controls, persistence, error handling, and deployment;
5. no unresolved release-gate defects or undocumented compatibility gaps; and
6. reviewed release notes and compatibility documentation that disclose every
   approved variance, removed feature, and deferred feature.

## Change Control

Adding or removing a 1.0 feature, weakening an acceptance gate, changing the
legacy binary claim, or changing required architectures requires a reviewed
scope change. Each affected requirement and milestone allocation shall be
updated in the same controlled change.

## References

- [Roadmap to 1.0.0](roadmap-1.0.md)
- [MP-0001 — Foundation and Feasibility](MP-0001-foundation-and-feasibility.md)
- [Project requirements](../requirements/README.md)
- [Reference environment](reference-environment.md)

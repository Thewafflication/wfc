# WFC Roadmap to 1.0.0

**Content type:** Product milestone roadmap
**Status:** Proposed
**Roadmap horizon:** Foundation through WFC 1.0.0
**Owner:** Project maintainer
**Approval:** Pending

## Roadmap Objective

Deliver the [WFC 1.0 compatibility profile](compatibility-profile-1.0.md) through
small, evidence-gated milestones. A version identifies the first baseline in
which the milestone capability is usable; it does not imply compatibility in
areas allocated to later milestones.

Every milestone shall have its own WSP work plan, reviewed entry baseline,
requirements allocation, tests, retained evidence, closeout, and successor
plan. A milestone may be replanned as evidence develops, but its required exit
gate may not be bypassed by moving unfinished work into an undocumented gap.

## Milestone Map

| Milestone | Target | Capability baseline | Required exit gate |
| --- | --- | --- | --- |
| MP-0001 | 0.1.0 | Foundation and feasibility | Controlled scope, architecture, dependencies, build/test skeleton, and one vertical slice |
| MP-0002 | 0.2.0 | Core VB/VBA language execution | Non-visual projects parse, bind, execute, and pass the core language/VBA reference suite |
| MP-0003 | 0.3.0 | Classes, projects, and Automation | Multi-file projects, class modules, events, COM identity, and Automation work end to end |
| MP-0004 | 0.4.0 | Forms and intrinsic controls | Ordinary/MDI forms and retained intrinsic controls load, interact, persist, and pass behavioral probes |
| MP-0005 | 0.5.0 | Component authoring, `stdole`, and OLE2 | ActiveX components, font/picture contracts, persistence, registration, and retained OLE hosting work |
| MP-0006 | 0.6.0 | `MSComctlLib` compatibility | Nine common controls satisfy API, behavior, event, rendering, and persistence gates |
| MP-0007 | 0.7.0 | Native compiler and VBRUN ABI | WFC produces native programs and the approved x86 legacy runtime ABI passes import/call probes |
| MP-0008 | 0.8.0 | Project compatibility and target matrix | Representative VB6 projects build and run on x86, x64, and ARM64 with documented variances |
| MP-0009 | 0.9.0 | Compatibility closure candidate | Applicable requirements are implemented; remaining work is defect, evidence, documentation, and release closure |
| MP-0010 | 1.0.0 | Mostly complete replacement | The 1.0 compatibility profile and all release-quality gates are approved |

## Initial Requirement Allocation

This allocation places the current requirements at the first milestone that
must satisfy them. A detailed milestone plan may split execution into smaller
work packages but shall not drop an allocated requirement without updating its
controlled disposition.

| Milestone | Existing allocation | Additional requirements to specify |
| --- | --- | --- |
| MP-0001 / 0.1.0 | Static discovery evidence and project-level foundation objectives | Build, architecture, dependency, probe-harness, test-dispatch, and vertical-slice requirements |
| MP-0002 / 0.2.0 | `REQ-0069`–`REQ-0094` VBA API contracts | VB language, VBA behavior, core runtime values, diagnostics, `.vbp/.bas`, and approved legacy dispositions |
| MP-0003 / 0.3.0 | Service/object contracts `REQ-0051`, `REQ-0052`, `REQ-0058`, `REQ-0067`, and `REQ-0068` | Classes, Automation, COM ABI, `Licenses`, retained `Control`, project references, and service-class behavior |
| MP-0004 / 0.4.0 | Visual/form contracts `REQ-0037`–`REQ-0050`, `REQ-0053`–`REQ-0062`, and behaviors `REQ-0106`–`REQ-0131` as retained | Form/resource formats, defaults, accessibility, rendering, and any replacement requirement created by a legacy removal |
| MP-0005 / 0.5.0 | OLE/component contracts `REQ-0063`–`REQ-0066` as retained and `stdole` `REQ-0095`–`REQ-0098` | Component persistence, registration, licensing, property bags, OLE2 behavior, and approved removal diagnostics |
| MP-0006 / 0.6.0 | `MSComctlLib` `REQ-0001`–`REQ-0036` | Additional defaults, persistence, accessibility, DPI, theme, and error requirements discovered by probes |
| MP-0007 / 0.7.0 | VBRUN `REQ-0099`–`REQ-0105` | Export signatures, calling conventions, layouts, helper behavior, native compiler ABI, and legacy-binary corpus |
| MP-0008 / 0.8.0 | Applicable requirements carried from subsystem milestones | Real-project, deployment, architecture, migration, compatibility-diagnostic, performance, and security requirements |
| MP-0009 / 0.9.0 | All applicable 1.0 requirements | Closure-only requirements for complete evidence, user information, release candidate, and defect gates |
| MP-0010 / 1.0.0 | Approved 1.0 requirement baseline | Release readiness, reproducibility, provenance, publication, rollback, and maintenance acceptance |

`___MSJetSQLHelp` is removed by the 1.0 compatibility profile and receives no
implementation requirement. Its unsupported-feature diagnostic shall be
specified during MP-0002 and verified no later than MP-0008.

## MP-0001 — Foundation and Feasibility (`0.1.0`)

### Outcome

A reproducible native project exists with controlled WSP/WCRT dependencies,
architecture and toolchain decisions, a reference-probe/test structure, and one
source-to-result vertical slice.

### Principal Work

- close the static discovery inventory, including all retained `VB6.OLB`
  interfaces and explicit JetSQL removal;
- record WSP adoption and WCRT integration decisions;
- create top-level CMake, warnings, analysis, target, and test dispatchers;
- define the intermediate representation and major component boundaries; and
- build one minimal VB source parse/bind/execute path.

### Exit Gate

All MP-0001 exit criteria pass, `0.1.0` scope is documented, and MP-0002 has a
reviewed work plan based on measured feasibility evidence.

## MP-0002 — Core VB/VBA Language Execution (`0.2.0`)

### Outcome

Non-visual VB6 modules can execute useful programs using the core VBA library
with reference-equivalent values, conversions, control flow, errors, files,
dates, strings, arrays, variants, and locale behavior.

### Principal Work

- lexical, grammar, semantic, symbol, type, and diagnostic specifications;
- procedures, module state, expressions, statements, arrays, UDTs, variants,
  strings, errors, and conditional compilation;
- core VBA library behavior and deterministic reference probes;
- `.vbp`, `.bas`, and module dependency loading; and
- disposition DDE, DAO/Data, OLE1, ActiveX Documents, PropertyPage hosting, and
  legacy help candidates before later subsystem gates.

### Exit Gate

The defined core-language corpus and applicable VBA requirements pass on the
initial target, diagnostics are stable and documented, and no legacy candidate
needed by MP-0003 or MP-0004 lacks a controlled disposition.

## MP-0003 — Classes, Projects, and Automation (`0.3.0`)

### Outcome

Multi-file projects with class modules, objects, properties, events, interfaces,
collections, and Automation clients/servers execute end to end.

### Principal Work

- class lifetime, default instances, `WithEvents`, `Implements`, properties,
  enumerators, and object/error semantics;
- COM identity, type information, `IUnknown`, `IDispatch`, connection points,
  variants, safe arrays, registration, activation, and threading decisions;
- `.cls` and component project metadata; and
- API/behavior closure for `App`, `Global`, `Screen`, `Clipboard`, `Licenses`,
  `Control`, and `VBControlExtender` as applicable.

### Exit Gate

Reference VB6 and WFC Automation programs exchange objects, invoke early and
late-bound members, raise events, preserve lifetime, and pass the allocated
project/class conformance suite.

## MP-0004 — Forms and Intrinsic Controls (`0.4.0`)

### Outcome

Ordinary and MDI VB6 form projects load, display, interact, save state, and run
using the intrinsic-control subset retained by the 1.0 profile.

### Principal Work

- `.frm/.frx` parsing, resource persistence, form/default-instance lifecycle,
  modality, MDI, menus, focus, validation, keyboard/mouse input, DPI, themes,
  accessibility, drawing, and printing;
- containers, z-order, tab order, control arrays, data binding where retained,
  timers, drag/drop, and intrinsic-control families; and
- execution of `TC-0106` through `TC-0131` plus service-class probes.

### Exit Gate

Every retained form/control behavioral requirement has VB6 reference evidence
and passing WFC conformance tests on the required milestone matrix. Removed
legacy controls or behaviors produce their specified diagnostics.

## MP-0005 — Component Authoring, `stdole`, and OLE2 (`0.5.0`)

### Outcome

WFC can build and host retained ActiveX component types and interoperate with
standard OLE font, picture, persistence, registration, and object-hosting APIs.

### Principal Work

- `UserControl`, retained `PropertyPage`/`UserDocument` behavior, property bags,
  ambient properties, licensing, persistence, and component events;
- `StdFont`, `StdPicture`, `IFont`, `IPicture`, picture load/save/render, and
  connection-point notifications;
- ActiveX DLL/EXE/OCX metadata, registration, activation, and packaging; and
- retained OLE2 container scenarios, explicitly excluding approved OLE1 or
  legacy features.

### Exit Gate

Reference and WFC containers can exchange, persist, activate, and use retained
components with matching COM identity, events, pictures, fonts, errors, and
lifetime on the required milestone matrix.

## MP-0006 — `MSComctlLib` Compatibility (`0.6.0`)

### Outcome

The TabStrip, Toolbar, StatusBar, ProgressBar, TreeView, ListView, ImageList,
Slider, and ImageCombo are usable by supported VB6 projects.

### Principal Work

- complete API/COM identities, collections, enumerations, default members,
  events, images, keyboard/mouse behavior, customization, editing, sorting,
  persistence, rendering, DPI, accessibility, and theming; and
- execute the existing `TC-0010` through `TC-0036` reference/conformance set
  plus tests required by the nine API contracts.

### Exit Gate

All applicable MSComctl requirements pass on the required milestone matrix and
representative forms load persisted controls without manual source changes.

## MP-0007 — Native Compiler and VBRUN ABI (`0.7.0`)

### Outcome

WFC produces deployable native programs, and the approved x86 legacy-runtime
profile can satisfy selected unmodified VB6 native binaries.

### Principal Work

- native code generation, calling conventions, stack/unwind behavior, startup,
  module initialization, exception/error flow, object and array layouts, and
  architecture-specific lowering;
- signatures and behavior for required compiler, member, language, numeric,
  project-hosting, and COM helpers documented in `REQ-0099` through `REQ-0105`;
- x86 `MSVBVM60.DLL` import/ordinal compatibility and safe process isolation;
  and
- packaging and dependency policy for x86, x64, and ARM64 native outputs.

### Exit Gate

WFC-built native applications pass the compiler/runtime corpus on all required
architectures, and the approved legacy x86 binary corpus resolves and executes
its runtime imports with documented limits.

## MP-0008 — Project Compatibility and Target Matrix (`0.8.0`)

### Outcome

Representative real VB6 applications build and run across the supported target
matrix with predictable migration diagnostics and no test-only integration
paths.

### Principal Work

- remaining language/library/project-format closure;
- project references, resources, manifests, icons, version information,
  deployment, registration, command-line tools, and incremental builds;
- x86, x64, and ARM64 CI; Unicode/ANSI, DPI, accessibility, locale, security,
  and performance matrices; and
- migration and compatibility diagnostics for removed or unsupported features.

### Exit Gate

The reviewed real-project corpus builds reproducibly and passes its functional
acceptance tests on required architectures, with every variance traced to an
approved scope or defect record.

## MP-0009 — Compatibility Closure Candidate (`0.9.0`)

### Outcome

Feature development for the 1.0 profile is complete. Work after this gate is
limited to defects, conformance evidence, hardening, documentation, packaging,
and approved release closure.

### Principal Work

- close the complete requirement/interface/feature inventory;
- execute full regression, reference-conformance, real-project, architecture,
  accessibility, performance, security, static-analysis, and recovery suites;
- resolve all release-gate defects and nondeterminism;
- complete user, compatibility, deployment, and migration documentation; and
- produce a reproducible release candidate and readiness record.

### Exit Gate

Every 1.0 feature is verified or has an approved variance/removal/deferment;
there are no unresolved release-gate defects, and the exact `1.0.0` candidate
baseline is nominated.

## MP-0010 — Mostly Complete Replacement (`1.0.0`)

### Outcome

WFC satisfies the approved 1.0 compatibility profile and is released as a
mostly complete replacement for the supported VB6 toolchain/runtime scope.

### Principal Work

- independently review the nominated source, dependencies, generated files,
  binaries, tests, evidence, documentation, licenses, security posture, and
  release automation;
- reproduce every release artifact from the controlled baseline;
- publish checksums, provenance, compatibility statement, release notes, and
  known limitations; and
- approve or reject the release without adding unverified feature work.

### Exit Gate

The release-readiness record approves the exact commit and artifacts, all 1.0
gates pass, published documentation matches the implementation, and recovery
and maintenance ownership are established.

## Cross-Milestone Controls

- Every requirement shall be allocated to one milestone or explicitly removed
  or deferred.
- Reference probes precede compatibility implementation when behavior is not
  established by a stable public contract.
- Each milestone preserves successful and failing evidence needed to justify
  scope, architecture, and compatibility decisions.
- x86 remains the VB6 reference architecture; x64 and ARM64 compatibility is
  introduced deliberately and may not change source-visible behavior without
  an approved variance.
- A milestone version shall not be tagged until its closeout and release or
  internal-baseline decision is approved.

## Immediate Planning Queue

Before MP-0001 closes:

1. adopt and pin WSP and WCRT;
2. close the `VB6.OLB` interface inventory except approved JetSQL removal;
3. allocate all existing requirements to the roadmap milestones;
4. establish the CMake, test, evidence, and reference-probe skeleton;
5. select and implement the vertical feasibility slice; and
6. baseline the detailed MP-0002 plan using the resulting measurements.

## References

- [WFC 1.0 compatibility profile](compatibility-profile-1.0.md)
- [MP-0001 — Foundation and Feasibility](MP-0001-foundation-and-feasibility.md)
- `wsp/processes/milestone-plan-template.md`
- `wsp/processes/milestone-closeout-template.md`
- `wsp/processes/release-readiness-template.md`

# WFC Planning

**Content type:** Project planning index

## Product Roadmap

- [1.0 compatibility profile](compatibility-profile-1.0.md) defines what a
  mostly complete replacement means and records explicit exclusions.
- [Legacy feature disposition proposals](legacy-feature-dispositions.md) draft
  recommendations for the six legacy features awaiting maintainer decision.
- [Roadmap to 1.0.0](roadmap-1.0.md) divides delivery into versioned milestones
  from foundation through compatibility closure and release.

## Completed Milestone

- [MP-0001 — Foundation and Feasibility](MP-0001-foundation-and-feasibility.md)
  established the `0.1.0` baseline.
- [WSP adoption record](wsp-adoption-record.md) controls the project process
  baseline and initial dispositions.
- [MP-0001 test strategy](test-strategy.md) defines the initial verification
  and evidence boundary.

## Active Milestone

MP-0009 — Native Compilation (`0.9.0`), per the
[MP-0009 plan](MP-0009-native-compilation.md) and the
[roadmap](roadmap-1.0.md). The plan awaits approval; no code has started.

Closed milestones:

- MP-0002 — Core VB/VBA Language Execution (`0.2.0`), closed 2026-10-10; see
  the [closeout record](MP-0002-closeout.md) and [work log](work-log-mp-0002.md).
- MP-0003 — Classes, Projects, and Automation (`0.3.0`), closed 2026-10-11 with
  COM server hosting deferred to MP-0005; see the
  [closeout record](MP-0003-closeout.md), [plan](MP-0003-classes-projects-automation.md)
  and [work log](work-log-mp-0003.md).
- MP-0004 — Forms and Intrinsic Controls (`0.4.0`), closed 2026-10-11 with
  reference probes, OLE drag and drop, and several drawing items deferred; see
  the [closeout record](MP-0004-closeout.md), [plan](MP-0004-forms-and-controls.md)
  and [work log](work-log-mp-0004.md).
- MP-0005 — Component Authoring, `stdole`, and OLE2 (`0.5.0`), closed 2026-10-12
  with in-process control servers, `.frx` persistence of hosted controls and
  real OLE drag deferred; see the [closeout record](MP-0005-closeout.md),
  [plan](MP-0005-component-authoring.md) and [work log](work-log-mp-0005.md).
- MP-0006 — `MSComctlLib` Compatibility (`0.6.0`), closed 2026-10-12 with
  MSCOMCTL binary persistence and reference probes deferred; see the
  [closeout record](MP-0006-closeout.md), [plan](MP-0006-mscomctllib.md),
  [coverage matrix](MP-0006-coverage.md) and [work log](work-log-mp-0006.md).
- MP-0007 — Packaging and VBRUN Export Surface (`0.7.0`), closed 2026-10-12
  with machine-code generation moved to MP-0009 (native x86, AMD64 and ARM64,
  no translation layer); see the [closeout record](MP-0007-closeout.md),
  [plan](MP-0007-native-compiler.md) and [work log](work-log-mp-0007.md).
- MP-0008 — Project Compatibility (`0.8.0`), closed 2026-10-20 with native
  machine code and reference-machine probes deferred; the first release
  (`0.8.0`) follows; see the [closeout record](MP-0008-closeout.md),
  [plan](MP-0008-project-compatibility.md) and [work log](work-log-mp-0008.md).

The [language requirements index](../requirements/language/README.md) is the
authoritative list of what has been specified and implemented, `REQ-0069`
through the current `REQ-0295`.

Detailed successor milestone plans shall be baselined during predecessor
closeout. The roadmap fixes their objective, target version, dependencies, and
exit gate; the detailed plan controls the work accepted into an active
milestone.

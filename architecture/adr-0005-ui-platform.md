# ADR-0005: UI Platform Abstraction and Test Hosts

**Content type:** Architecture decision record

**Status:** Accepted

**Date:** 2026-10-11

## Context

MP-0004 adds forms and intrinsic controls. The architecture forbids the parser,
binder and interpreter from calling Win32 or UI APIs directly, requires
deterministic tests for most behavior, and requires native execution evidence
on x86, x64 and ARM64. Form and control behavior (event order, focus, validation,
modality, coordinates) is mostly logic that does not need pixels, while
rendering, DPI and input need a real window system.

## Decision Drivers

- Deterministic, fast CTest coverage of event order and control logic.
- Real Win32 behavior (focus, painting, DPI, accessibility) on all targets.
- One implementation of control logic, not one per backend.
- Re-entrant event delivery into a single-threaded interpreter.
- No dependency on a third-party GUI toolkit.

## Considered Options

1. **Call Win32 from the control code directly.** Simplest, but untestable
   without a desktop and violates the architecture dependency rule.
2. **A third-party toolkit (Qt, wxWidgets, ...).** Portable, but adds a large
   dependency, differs from VB6 control behavior, and complicates ARM64/x86
   builds and licensing.
3. **An abstract `UiHost` with a Win32 implementation and a scripted headless
   implementation.** Control logic is written once against the interface.

## Decision

Take option 3.

- `UiHost` (namespace `wfc::ui`) is a small abstract interface: create/destroy
  top-level and child windows, set geometry and visibility, set text and state,
  post and pump events, timers, focus and capture, clipboard text, drawing
  surface operations (lines, fills, text, bitmaps), common dialogs, and monitor
  metrics. It speaks twips and logical units; DPI conversion is the host's job.
- The **headless host** keeps window state in memory, records every drawing
  operation as a trace, and executes a script of input events (click, key,
  focus change, timer tick) so tests can assert event order and results.
- The **Win32 host** maps windowed controls to standard Win32 classes
  (`BUTTON`, `EDIT`, `LISTBOX`, `COMBOBOX`, `SCROLLBAR`, `STATIC`), draws
  lightweight controls itself, and runs the message loop. Its messages become
  `UiHost` events delivered to the control model.
- Control logic and the object model sit above `UiHost` and reach the
  interpreter through the same re-entrant event path used for COM events.
- Control property, method and event tables are generated from the retained
  type-library evidence rather than written by hand.
- A host is chosen at start-up: Win32 for `wfc` on Windows by default,
  headless for tests and when `--headless` is given.

## Consequences

- Most conformance tests run in the plain CTest process with no desktop.
- A small, serialized Win32 integration set verifies the real host; it is
  reported as skipped, never passed, when no desktop is available.
- Anything the interface cannot express (for example a new OS feature) forces
  an interface change, which is reviewed like any other API change.
- The interface becomes a compatibility surface for later milestones
  (MSComctlLib, ActiveX controls), so it must stay small and documented.

## Rationale

The split matches the existing architecture rule, keeps behavior testable and
reproducible, and avoids a heavyweight dependency while still using the native
controls that give VB6 forms their look and feel.

## References

- [MP-0004 plan](../planning/MP-0004-forms-and-controls.md)
- [Architecture overview](README.md)
- [Form and intrinsic-control behavioral requirements](../requirements/vb/behavior/README.md)

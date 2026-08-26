# REQ-0114 — Manual and OLE drag-and-drop behavior

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to Forms and intrinsic controls exposing drag or OLE drag/drop members on supported Windows targets.

## Requirement

WFC shall implement manual drag states and `DragDrop`/`DragOver` event transitions, plus OLE drag initiation, format negotiation, delayed data supply, effects, feedback, completion, and drop events. ByRef `Effect`, `State`, and cursor-selection values shall control subsequent behavior; disabled OLE modes shall reject the corresponding operation. Coordinates and source/data object identity shall match VB6.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0114` VB6 reference and WFC conformance test

Trace enter/over/leave/drop and cancel sequences within and across controls and processes, negotiate text and binary formats, request delayed data, vary allowed effects and ByRef mutations, and compare cursors, coordinates, event order, and errors.

The probe shall retain inputs, event traces, values, errors, and visual evidence
needed to distinguish the stated cases. WFC shall match the confirmed reference
result unless an approved tailoring decision states otherwise.

## Relationships

- **Derived from:** `evidence/reference/vb-6.0-type-library.json` and
  `evidence/reference/vb6-form-control-documentation.md`
- **Depends on:** Applicable VB class API contracts in `REQ-0037` through `REQ-0068`
- **Conflicts with:** None known

## Tailoring

A documented platform difference may be accepted only through an approved
compatibility-scope or architecture decision that identifies its user impact.

## Implementation Record

Not implemented.

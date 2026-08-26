# REQ-0110 — Form window geometry, state, and MDI behavior

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to `Form` and `MDIForm` on supported Windows targets.

## Requirement

WFC shall apply border, control-box, minimize/maximize, taskbar, startup-position, moveability, window-state, icon, caption, and geometry properties with VB6 design-time versus run-time mutability. Geometry and window-state transitions shall raise `Resize` in the reference cases. An `MDIForm` shall contain MDI-child forms, maintain `ActiveForm`, arrange children, implement `AutoShowChildren` and scroll-bar behavior, and constrain MDI-child visibility, ownership, and activation as the reference does.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0110` VB6 reference and WFC conformance test

Exercise every border and startup style, minimize/maximize/restore, programmatic and interactive moves, taskbar presence, MDI child load/show/hide/unload, activation, arranging, scrolling, and invalid MDI configurations.

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

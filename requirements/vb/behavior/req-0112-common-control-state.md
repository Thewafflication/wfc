# REQ-0112 — Common intrinsic-control state and interaction

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to All visual intrinsic controls on supported Windows targets.

## Requirement

Each intrinsic control shall apply `Enabled`, `Visible`, position, size, `TabIndex`, `TabStop`, mouse-pointer/icon, tooltip, help context, font, color, appearance, right-to-left, tag, and container properties where its API exposes them. `Move`, `ZOrder`, `Refresh`, `SetFocus`, and mouse, keyboard, click, double-click, drag, and focus events shall match VB6 availability, ordering, coordinate units, ByRef mutation, and error behavior. Disabling or hiding a focused control shall transfer focus as in the reference.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0112` VB6 reference and WFC conformance test

Use a matrix across every visual intrinsic class for property defaults and mutation, tab navigation, focus eligibility, z-order, refresh, input event order, coordinate values, ByRef cancellation, visibility, disabled input, and invalid values.

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

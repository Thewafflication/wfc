# REQ-0120 — CommandButton activation behavior

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to `CommandButton` on supported Windows targets.

## Requirement

`CommandButton` shall raise `Click` for reference-equivalent mouse, keyboard, mnemonic, programmatic `Value`, default-button Enter, and cancel-button Escape activation. Exactly the eligible `Default` and `Cancel` command buttons for the active form shall receive those keys. Standard and graphical styles shall apply normal, disabled, and down pictures, mask color, caption, focus cue, and pressed state as in VB6. `CausesValidation` shall govern pre-click validation.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0120` VB6 reference and WFC conformance test

Activate by every input path with focus on different controls, accepted/cancelled validation, enabled/visible changes, multiple default/cancel assignments, and all graphical pictures/masks; compare event count/order and pixels.

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

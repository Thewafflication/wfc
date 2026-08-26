# REQ-0109 — Activation, focus, validation, and key preview

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to `Form`, `MDIForm`, and focusable intrinsic controls on supported Windows targets.

## Requirement

WFC shall track one active form and one active control per applicable container, raise activation/deactivation and focus events when those states change, and expose them through `ActiveForm` and `ActiveControl`. Before focus leaves a control, WFC shall raise `Validate(Cancel)` when the destination causes validation; cancellation shall retain focus and suppress the remaining transition as in VB6. A form with `KeyPreview = True` shall receive keyboard events before the focused control, with ByRef key mutations affecting subsequent dispatch.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0109` VB6 reference and WFC conformance test

Trace keyboard, mouse, `SetFocus`, hide, disable, unload, and cross-form transitions with validation accepted, cancelled, and bypassed by `CausesValidation = False`; mutate `KeyCode` and `KeyAscii` in form handlers.

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

# REQ-0107 — Form visibility, ownership, and modality

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to `Form` and `MDIForm` on supported Windows targets.

## Requirement

`Load` shall create a form without requiring it to be visible; `Show` shall load an unloaded form and make it visible; `Hide` shall remove it from display without unloading it or discarding state. Modal `Show` shall suspend the calling VB execution path until the form is hidden or unloaded, disable the applicable owner window while modal, and restore owner activation when the modal operation ends. Modeless and owned-form behavior shall follow the reference result for each accepted `Modal` and `OwnerForm` argument.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0107` VB6 reference and WFC conformance test

Trace visibility, state retention, caller blocking, owner enabled/active state, nesting, and invalid modal/owner arguments for loaded and unloaded forms.

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

# REQ-0108 — Form query, unload, and termination

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to `Form` and `MDIForm` on supported Windows targets.

## Requirement

An ordinary close or `Unload` operation shall raise `QueryUnload` before `Unload`, populate `UnloadMode` with the reference reason, and honor each `Cancel` argument at the stage where VB6 permits cancellation. Successful unloading shall destroy the window and loaded controls while preserving any independently held form-object reference as in VB6. `Terminate` shall occur only when the form object is finally released. Abrupt `End` termination is outside this orderly event path.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0108` VB6 reference and WFC conformance test

Close forms by control menu, code, owner/MDI shutdown, task-manager/session shutdown where safely reproducible, and application exit; cancel each event separately and trace window, object, and event lifetime.

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

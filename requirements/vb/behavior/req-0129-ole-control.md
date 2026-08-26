# REQ-0129 — Intrinsic OLE container behavior

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to `OLE` on supported Windows targets.

## Requirement

The OLE control shall host linked and embedded OLE objects using the reference class/type restrictions, creation and paste dialogs, source document/item, activation modes, verbs, size/display modes, update policy, data formats, clipboard operations, object movement, persistence, and close/delete lifecycle. Its `Object`, `LpOleObject`, application-running state, accepted/provided formats, actions, events, and errors shall preserve COM identity, reference counting, in-place activation, and storage interoperability required by VB6 containers.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0129` VB6 reference and WFC conformance test

Use deterministic in-process and out-of-process OLE test servers plus installed standard servers to exercise create/link/embed, verbs, in-place activation, update, clipboard, save/load formats, movement/resize, server failure, cancellation, close/delete, reference lifetime, and invalid classes/documents.

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

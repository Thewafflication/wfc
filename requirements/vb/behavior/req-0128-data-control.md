# REQ-0128 — Intrinsic Data control behavior

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to `Data` and data-bound intrinsic controls on supported Windows targets.

## Requirement

The intrinsic Data control shall implement the VB6/DAO connection, database, record-source, recordset, cursor, locking, edit, BOF/EOF action, refresh, navigation, update, and error model exposed by its API. Bound controls shall exchange values through `DataSource`, `DataMember`, `DataField`, `DataFormat`, and `DataChanged`; `UpdateControls` and `UpdateRecord` shall synchronize in the reference direction and raise `Validate`, `Reposition`, `Error`, and bound-control change events with reference cancellation and response semantics.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0128` VB6 reference and WFC conformance test

Use version-controlled DAO databases to test open/close, table/query/SQL sources, navigation and boundaries, add/edit/delete/update/cancel, validation and errors, nulls and conversions, formatting, multi-control binding, external record changes, read-only/exclusive modes, and teardown.

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

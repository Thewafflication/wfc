# REQ-0131 — Common intrinsic-control data binding

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to Intrinsic controls exposing `DataSource`, `DataMember`, `DataField`, `DataFormat`, or `DataChanged` on supported Windows targets.

## Requirement

A bound intrinsic control shall resolve its data source/member/field using VB6 rules, convert between field and control value using `DataFormat` and locale, report pending edits through `DataChanged`, refresh when the current record changes, and participate in update, validation, null, error, and teardown behavior without losing reference-equivalent edits. Unsupported provider or format features shall return a stable compatibility error.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0131` VB6 reference and WFC conformance test

Bind each eligible control to controlled DAO and Automation sources; cover navigation, edits from both sides, null/empty/error values, conversion and locale, validation cancellation, read-only fields, source replacement/destruction, invalid member/field/format, and multiple consumers.

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

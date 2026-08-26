# REQ-0115 — Form and intrinsic-control persistence

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to Forms, menus, and intrinsic controls on supported Windows targets.

## Requirement

WFC shall load the VB6 form description and companion binary resources needed to reconstruct form class identity, containment, control arrays, menus, design-time property values, fonts, colors, pictures, icons, and persisted control state. Properties omitted from source shall assume VB6 defaults. Unknown, malformed, versioned, or unsupported persisted values shall produce a controlled compatibility diagnostic rather than silent reinterpretation.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0115` VB6 reference and WFC conformance test

Save representative VB6 projects containing every form and intrinsic-control class, compare text and binary resource reconstruction, omit default properties, vary locale and line endings, and exercise malformed and unsupported records.

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

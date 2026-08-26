# REQ-0119 — Frame containment and option grouping

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to `Frame` on supported Windows targets.

## Requirement

`Frame` shall provide a visual and functional child-control container with VB6 clipping, coordinates, enabled/visible propagation, caption, border, appearance, and input behavior. Option buttons whose immediate container is the same frame shall form a mutually exclusive group independent of option buttons in other containers. Moving an existing control over a frame shall not change containment unless the VB6 designer/source representation does so.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0119` VB6 reference and WFC conformance test

Compare nested containment, child coordinates, clipping, enable/visible interaction, z-order, hit testing, caption/border rendering, option groups across multiple frames, and design-time versus run-time placement.

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

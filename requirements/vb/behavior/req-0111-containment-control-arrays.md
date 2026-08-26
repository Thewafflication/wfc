# REQ-0111 — Control containment, lookup, and control arrays

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to Form hosts, container controls, and all intrinsic controls exposing `Index` on supported Windows targets.

## Requirement

WFC shall preserve the design-time containment hierarchy exposed through `Parent`, `Container`, `Controls`, `Count`, and the form default member. Controls shall be retrievable using the VB6-supported name and index forms. Controls sharing a name and distinct `Index` values shall behave as a control array: event dispatch shall identify the originating index, `Load` shall create a new run-time element using reference property-copy and visibility rules, and `Unload` shall remove only elements eligible for dynamic removal while preserving index bounds and lookup errors.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0111` VB6 reference and WFC conformance test

Build nested frames and picture boxes, enumerate and index controls, test duplicate names and sparse indices, dynamically load/show/unload array elements, compare copied properties and tab order, and record invalid lookup/removal errors.

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

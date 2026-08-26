# REQ-0123 — Horizontal and vertical ScrollBar behavior

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to `HScrollBar` and `VScrollBar` on supported Windows targets.

## Requirement

Each scroll bar shall constrain `Value` to the inclusive `Min`/`Max` range, support reference reversed-range behavior, and apply `SmallChange` and `LargeChange` for arrow, keyboard, page, and track interactions. Interactive tracking shall raise `Scroll` and committed value changes shall raise `Change` in the VB6 order and frequency. Programmatic assignments, focus, keyboard direction, right-to-left behavior, range mutation, and invalid values shall match VB6.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0123` VB6 reference and WFC conformance test

Cover normal, equal, and reversed ranges; boundary clamping/errors; programmatic and every input path; held/repeated input; live thumb tracking; event order/count; focus; disable/hide; and right-to-left layout.

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

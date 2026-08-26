# REQ-0116 — PictureBox container and presentation behavior

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to `PictureBox` on supported Windows targets.

## Requirement

`PictureBox` shall act as a visual control and container, preserve child coordinates and clipping, display its `Picture`, produce its composed `Image`, apply `AutoSize`, border and appearance styles, and implement the drawing contract in REQ-0113. Picture mutation, resizing, repainting, data binding, and DDE/OLE interactions shall raise the reference events and retain pixels according to `AutoRedraw`.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0116` VB6 reference and WFC conformance test

Compare containment, clipping, autosize, picture versus image identity, bitmap/icon/metafile rendering, resize and paint traces, data changes, and drawing persistence.

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

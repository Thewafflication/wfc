# REQ-0125 — Image, Shape, and Line presentation behavior

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to `Image`, `Shape`, and `Line` on supported Windows targets.

## Requirement

The lightweight graphical controls shall render without independent window handles and participate in container z-order, clipping, visibility, and repainting as in VB6. `Image` shall render pictures using stretch, border, appearance, transparency, input, drag/drop, and data-binding rules. `Shape` shall render every supported shape with border, fill, background, and draw modes. `Line` shall render its endpoints with border color, style, width, and draw mode, and expose bounds derived from its coordinates as VB6 does.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0125` VB6 reference and WFC conformance test

Inspect window handles, hit testing and overlap with windowed controls; compare bitmap/icon/metafile images with stretch on/off; render every shape, line direction, width/style/fill/draw mode, clipping and resize; trace Image input.

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

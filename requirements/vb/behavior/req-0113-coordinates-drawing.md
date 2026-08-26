# REQ-0113 — Form and PictureBox coordinate and drawing behavior

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to `Form` and `PictureBox` on supported Windows targets.

## Requirement

WFC shall implement VB6 logical coordinate systems through `ScaleMode`, `ScaleLeft`, `ScaleTop`, `ScaleWidth`, `ScaleHeight`, `Scale`, `ScaleX`, and `ScaleY`. Drawing state and operations including current position, colors, modes, styles, widths, fills, `AutoRedraw`, `ClipControls`, `Cls`, `Circle`, `Line`, `PaintPicture`, `Point`, `PSet`, text measurement, `Image`, `Picture`, `hDC`, `HasDC`, `Paint`, and `PrintForm` shall produce reference-equivalent pixels, units, persistence, repainting, and errors.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0113` VB6 reference and WFC conformance test

Render deterministic fixtures under every supported scale and draw mode to retained bitmaps; cover negative dimensions, clipping, resize/repaint, AutoRedraw on/off, image/picture snapshots, text metrics, device-context lifetime, and invalid arguments.

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

# REQ-0035 — ImageCombo item presentation

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Microsoft Common Controls 6.0 installed help, `vbctlimagecombocontrol.htm`; `vbobjcomboitemobject.htm`; `vbmthaddmethod(comboitemscollection).htm`

## Scope

This requirement applies to the presentation of ImageCombo items on supported Windows targets.

## Requirement

Each WFC ImageCombo item shall display its configured text, resolve its normal
and selected images by ImageList index or key, and offset its presentation by
its indentation level using the control's indentation distance.

## Rationale

Per-item imagery and indentation distinguish ImageCombo from the standard VB6
combo box.

## Verification

**Method:** Test

**References:** Planned `TC-0035` VB6 reference and WFC conformance test

The test shall compare text, normal and selected imagery, key/index resolution,
and multiple indentation levels against the installed reference baseline.

## Relationships

- **Derived from:** Installed `cmctl198.chm` documentation and `REQ-0009`
- **Depends on:** `REQ-0028` and the ImageCombo API contract
- **Conflicts with:** None known

## Tailoring

A documented platform difference may be accepted only through an approved
compatibility-scope or architecture decision that identifies its user impact.

## Implementation Record

Not implemented.

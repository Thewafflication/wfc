# REQ-0029 — ImageList rendering and composition

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Microsoft Common Controls 6.0 installed help, `vbmthdraw.htm`; `vbmthextracticon.htm`; `vbmthoverlay.htm`

## Scope

This requirement applies to drawing, extracting, and overlaying ImageList images on supported Windows targets.

## Requirement

The WFC ImageList shall draw images in normal, mask-transparent, selected, and focus styles; extract a usable icon from an image; and overlay two index- or key-selected images using `MaskColor` for transparency and return the composite picture.

## Rationale

These operations are the observable output behaviors beyond using the control as an image repository.

## Verification

**Method:** Test

**References:** Planned `TC-0029` VB6 reference and WFC conformance test

The test shall exercise each stated condition against the installed reference
baseline and WFC, retain the observable results, and require equivalent results
unless an approved tailoring decision states otherwise.

## Relationships

- **Derived from:** The installed `cmctl198.chm` documentation and the relevant
  Automation contract requirement in `REQ-0001` through `REQ-0009`
- **Depends on:** Reference environment record and the applicable control API contract
- **Conflicts with:** None known

## Tailoring

A documented platform difference may be accepted only through an approved
compatibility-scope or architecture decision that identifies its user impact.

## Implementation Record

Not implemented.


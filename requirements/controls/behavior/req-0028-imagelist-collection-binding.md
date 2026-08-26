# REQ-0028 — ImageList collection and binding

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Microsoft Common Controls 6.0 installed help, `vbobjimagelist.htm`; `vbmthaddlistimages.htm`

## Scope

This requirement applies to the ImageList `ListImages` collection and binding to another common control on supported Windows targets.

## Requirement

The WFC ImageList shall expose a one-based image collection addressable by index or unique key, return the newly added image, make image dimensions read-only after the first addition, and after binding prohibit deletion or middle insertion while permitting append operations.

## Rationale

Stable indexing and post-binding restrictions protect image references held by consuming controls.

## Verification

**Method:** Test

**References:** Planned `TC-0028` VB6 reference and WFC conformance test

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


# REQ-0018 — ProgressBar range and rendering

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Microsoft Common Controls 6.0 installed help, `vbobjprogressbar.htm`

## Scope

This requirement applies to ProgressBar `Min`, `Max`, and `Value` state on supported Windows targets.

## Requirement

The WFC ProgressBar shall represent progress with an inclusive range bounded by `Min` and `Max`, position it using `Value`, and render a filled extent that approximates the value using chunks determined by the control dimensions and border.

## Rationale

The reference explicitly distinguishes numerical state from the chunked visual approximation.

## Verification

**Method:** Test

**References:** Planned `TC-0018` VB6 reference and WFC conformance test

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


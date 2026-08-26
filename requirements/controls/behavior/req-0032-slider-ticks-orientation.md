# REQ-0032 — Slider ticks and orientation

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Microsoft Common Controls 6.0 installed help, `vbproorientationslider.htm`; `vbprotickstyle.htm`; `vbprotickfrequency.htm`; `vbmthgetnumticks.htm`; `vbmthclearsel.htm`

## Scope

This requirement applies to Slider orientation and tick presentation on supported Windows targets.

## Requirement

The WFC Slider shall support horizontal and vertical orientations, position ticks on the bottom/right, top/left, both sides, or neither, space ticks according to `TickFrequency`, report their count through `GetNumTicks`, and clear a selection by setting `SelStart` to `Value` and `SelLength` to zero.

## Rationale

The tick and orientation settings define the slider's scale and visual geometry.

## Verification

**Method:** Test

**References:** Planned `TC-0032` VB6 reference and WFC conformance test

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


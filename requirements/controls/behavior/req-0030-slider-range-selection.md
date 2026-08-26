# REQ-0030 — Slider value and selection range

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Microsoft Common Controls 6.0 installed help, `vbobjslider.htm`; `vbproselectrange.htm`; `vbprosellengthslider.htm`

## Scope

This requirement applies to Slider range, selection, and tick state on supported Windows targets.

## Requirement

The WFC Slider shall constrain `Value`, `SelStart`, and `SelLength` to its `Min`/`Max` range, require nonnegative `SelLength` with `SelStart + SelLength` not exceeding `Max`, couple `SelStart` and `Value` while `SelectRange` is false, and make `SelLength` ineffective in that mode.

## Rationale

These invariants define the numerical state represented by the slider.

## Verification

**Method:** Test

**References:** Planned `TC-0030` VB6 reference and WFC conformance test

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


# REQ-0016 — StatusBar panel model

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Microsoft Common Controls 6.0 installed help, `vbobjstatusbar.htm`; `vbcolpanelscollection.htm`; `vbobjpanel.htm`

## Scope

This requirement applies to a StatusBar in normal mode on supported Windows targets.

## Requirement

The WFC StatusBar shall expose a `Panels` collection containing no more than sixteen panels, with each panel capable of displaying text, a picture, or both and honoring its width, alignment, bevel, minimum-width, and autosize settings.

## Rationale

The panel model is the principal means by which VB6 applications present multiple categories of status information.

## Verification

**Method:** Test

**References:** Planned `TC-0016` VB6 reference and WFC conformance test

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


# REQ-0010 — TabStrip collection and selection

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Microsoft Common Controls 6.0 installed help, `vbobjtabcontrol.htm`; `vbcoltabscollection.htm`; `vbmthaddtabs.htm`

## Scope

This requirement applies to the TabStrip control and its `Tabs` and `Tab` objects on supported Windows targets.

## Requirement

The WFC TabStrip shall expose its tabs as a one-based `Tabs` collection, permit tabs to be added and removed at run time, permit each tab to be addressed by index or key, and report the active tab through `SelectedItem`.

## Rationale

This is the documented object model used by VB6 applications to construct and select tabbed pages.

## Verification

**Method:** Test

**References:** Planned `TC-0010` VB6 reference and WFC conformance test

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


# REQ-0017 — StatusBar normal and simple modes

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Microsoft Common Controls 6.0 installed help, `vbprostylestatusbar.htm`; `vbprosimpletext.htm`; `vbprostylepanel.htm`

## Scope

This requirement applies to the StatusBar `Style`, `SimpleText`, and Panel `Style` properties on supported Windows targets.

## Requirement

In normal mode the WFC StatusBar shall display its panel collection; in simple mode it shall display one full-width raised panel containing `SimpleText`, independently preserve the normal and simple appearances, and ignore the `Panels` collection for the displayed simple panel.

## Rationale

Applications temporarily switch modes for contextual messages while retaining their normal panel layout.

## Verification

**Method:** Test

**References:** Planned `TC-0017` VB6 reference and WFC conformance test

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


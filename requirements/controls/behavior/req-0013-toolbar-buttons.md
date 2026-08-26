# REQ-0013 — Toolbar button behavior

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Microsoft Common Controls 6.0 installed help, `vbobjtoolbar.htm`; `vbmthaddbuttons.htm`; `vbprostylebutton.htm`

## Scope

This requirement applies to the Toolbar `Buttons`, `Button`, and `ButtonMenus` object model on supported Windows targets.

## Requirement

The WFC Toolbar shall support adding and removing buttons at run time and shall reproduce the six documented button styles: push button, check button, mutually exclusive button group, eight-pixel separator, width-adjustable placeholder, and dropdown menu button.

## Rationale

Toolbar behavior is defined primarily by its button collection and per-button style.

## Verification

**Method:** Test

**References:** Planned `TC-0013` VB6 reference and WFC conformance test

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


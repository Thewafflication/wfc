# REQ-0014 — Toolbar customization

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Microsoft Common Controls 6.0 installed help, `vbproallowcustomize.htm`; `vbmthcustomize.htm`

## Scope

This requirement applies to run-time end-user customization of a Toolbar on supported Windows targets.

## Requirement

When `AllowCustomize` is true, the WFC Toolbar shall open its customization interface on a toolbar double-click, and `Customize` shall open the same interface programmatically so the user can hide, display, or rearrange buttons.

## Rationale

Applications rely on both direct and policy-controlled access to the built-in customization workflow.

## Verification

**Method:** Test

**References:** Planned `TC-0014` VB6 reference and WFC conformance test

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


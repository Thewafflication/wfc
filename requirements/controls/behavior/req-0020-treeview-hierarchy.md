# REQ-0020 — TreeView hierarchy management

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Microsoft Common Controls 6.0 installed help, `vbobjtreeview.htm`; `vbmthaddnode.htm`

## Scope

This requirement applies to the TreeView `Nodes` collection and Node relationship model on supported Windows targets.

## Requirement

The WFC TreeView shall expose a one-based `Nodes` collection and add nodes at the first, last, next, previous, or child relationship to a referenced node; when no reference is supplied, it shall append the node at the top level.

## Rationale

The hierarchy and relative insertion rules determine stable tree construction from VB6 code.

## Verification

**Method:** Test

**References:** Planned `TC-0020` VB6 reference and WFC conformance test

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


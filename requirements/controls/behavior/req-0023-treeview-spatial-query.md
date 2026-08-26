# REQ-0023 — TreeView visibility and hit testing

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Microsoft Common Controls 6.0 installed help, `vbmthhittest.htm`; `vbmthensurevisible.htm`

## Scope

This requirement applies to TreeView Node visibility and coordinate queries on supported Windows targets.

## Requirement

`HitTest(x, y)` shall return the Node at the supplied coordinates or `Nothing`, and a Node's `EnsureVisible` shall expand and scroll as necessary and return whether any expansion or scrolling occurred; `EnsureVisible` shall not operate when `Scroll` is false.

## Rationale

Drag-and-drop and programmatic navigation depend on deterministic spatial lookup and visibility behavior.

## Verification

**Method:** Test

**References:** Planned `TC-0023` VB6 reference and WFC conformance test

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


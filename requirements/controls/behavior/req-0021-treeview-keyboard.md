# REQ-0021 — TreeView keyboard navigation

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Microsoft Common Controls 6.0 installed help, `vbobjtreeview.htm`

## Scope

This requirement applies to keyboard input while a TreeView has focus on supported Windows targets.

## Requirement

The WFC TreeView shall navigate expanded visible nodes with Up and Down, expand or advance with Right, collapse with Left, wrap from the bottom to the top, and cycle among visible nodes beginning with a typed ANSI character.

## Rationale

Keyboard behavior is externally observable and required for compatible interaction and accessibility.

## Verification

**Method:** Test

**References:** Planned `TC-0021` VB6 reference and WFC conformance test

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


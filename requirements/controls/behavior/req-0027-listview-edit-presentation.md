# REQ-0027 — ListView label editing

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Microsoft Common Controls 6.0 installed help, `vbprolabeledit.htm`; `vbevtbeforelabeledit.htm`; `vbevtafterlabeledit.htm`

## Scope

This requirement applies to editing the selected ListItem label on supported Windows targets.

## Requirement

The WFC ListView shall initiate automatic label editing on the second click of
a selected item, initiate manual editing only through `StartLabelEdit`, raise
`BeforeLabelEdit` after `Click`, permit a nonzero cancellation, and raise
`AfterLabelEdit` with the entered text or `Null` when the user cancels.

## Rationale

VB6 applications validate and accept item labels through this ordered,
cancellable event protocol.

## Verification

**Method:** Test

**References:** Planned `TC-0027` VB6 reference and WFC conformance test

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

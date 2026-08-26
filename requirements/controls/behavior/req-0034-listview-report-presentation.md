# REQ-0034 — ListView report presentation options

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Microsoft Common Controls 6.0 installed help, `vbprocheckboxesproperty.htm`; `vbprofullrowselectproperty.htm`; `vbprogridlinesproperty(listviewcontrol).htm`; `vbprohidecolumnheaders.htm`; `vbprohoverselectionproperty.htm`

## Scope

This requirement applies to ListView presentation and selection on supported Windows targets.

## Requirement

The WFC ListView shall honor checkbox visibility, full-row selection only in
Report view, gridline visibility only in Report view, column-header visibility,
and delayed selection when the pointer hovers over an item.

## Rationale

These settings are directly visible and govern common interaction patterns in
Report view.

## Verification

**Method:** Test

**References:** Planned `TC-0034` VB6 reference and WFC conformance test

The test shall exercise each stated condition against the installed reference
baseline and WFC, retain the observable results, and require equivalent results
unless an approved tailoring decision states otherwise.

## Relationships

- **Derived from:** Installed `cmctl198.chm` documentation and `REQ-0006`
- **Depends on:** Reference environment record and ListView API contract
- **Conflicts with:** None known

## Tailoring

A documented platform difference may be accepted only through an approved
compatibility-scope or architecture decision that identifies its user impact.

## Implementation Record

Not implemented.

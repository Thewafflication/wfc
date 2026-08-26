# REQ-0033 — ImageCombo collection insertion

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Microsoft Common Controls 6.0 installed help, `vbctlimagecombocontrol.htm`; `vbmthaddmethod(comboitemscollection).htm`

## Scope

This requirement applies to the ImageCombo `ComboItems` collection on supported Windows targets.

## Requirement

The WFC ImageCombo shall start with an empty `ComboItems` collection and, when
`Add` receives no arguments, append an item with zero indentation and return a
reference to that new item; collection items shall be addressable by their
one-based index or unique key.

## Rationale

The collection model and returned object reference are the documented means of
constructing an ImageCombo at run time.

## Verification

**Method:** Test

**References:** Planned `TC-0033` VB6 reference and WFC conformance test

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

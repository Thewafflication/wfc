# REQ-0036 — ImageCombo editing lock

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Microsoft Common Controls 6.0 installed help, `vbprolockedproperty.htm`

## Scope

This requirement applies to user editing of ImageCombo text on supported Windows targets.

## Requirement

The WFC ImageCombo shall permit the user to edit its text while `Locked` is
false and shall prevent user editing while `Locked` is true.

## Rationale

The lock state changes user interaction without preventing programmatic access.

## Verification

**Method:** Test

**References:** Planned `TC-0036` VB6 reference and WFC conformance test

The test shall attempt equivalent keyboard editing in both states against the
installed reference baseline and WFC.

## Relationships

- **Derived from:** Installed `cmctl198.chm` documentation and `REQ-0009`
- **Depends on:** ImageCombo API contract
- **Conflicts with:** None known

## Tailoring

None.

## Implementation Record

Not implemented.

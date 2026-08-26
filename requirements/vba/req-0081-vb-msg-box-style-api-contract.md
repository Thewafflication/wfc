# REQ-0081 — VBA VbMsgBoxStyle values

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic for Applications 6.0 type library (`VBA6.DLL`)

## Scope

This requirement applies wherever VBA exposes the `VbMsgBoxStyle` constants.

## Requirement

The WFC VBA runtime shall expose each listed constant with the specified name, value, and member identifier.

## Value Inventory

| Constant | Value | Member ID |
| --- | ---: | ---: |
| `vbOKOnly` | `0` | 1073741824 |
| `vbOKCancel` | `1` | 1073741825 |
| `vbAbortRetryIgnore` | `2` | 1073741826 |
| `vbYesNoCancel` | `3` | 1073741827 |
| `vbYesNo` | `4` | 1073741828 |
| `vbRetryCancel` | `5` | 1073741829 |
| `vbCritical` | `16` | 1073741830 |
| `vbQuestion` | `32` | 1073741831 |
| `vbExclamation` | `48` | 1073741832 |
| `vbInformation` | `64` | 1073741833 |
| `vbDefaultButton1` | `0` | 1073741834 |
| `vbDefaultButton2` | `256` | 1073741835 |
| `vbDefaultButton3` | `512` | 1073741836 |
| `vbDefaultButton4` | `768` | 1073741837 |
| `vbApplicationModal` | `0` | 1073741838 |
| `vbSystemModal` | `4096` | 1073741839 |
| `vbMsgBoxHelpButton` | `16384` | 1073741840 |
| `vbMsgBoxRight` | `524288` | 1073741841 |
| `vbMsgBoxRtlReading` | `1048576` | 1073741842 |
| `vbMsgBoxSetForeground` | `65536` | 1073741843 |

## Rationale

This contract is part of the source-visible VBA standard library required
to compile and execute compatible VB and VBA code.

## Verification

**Method:** Inspection

**References:** `evidence/reference/vba-6.0-type-library.json`; test case TBD

Inspect the WFC VBA type information and compare every listed name, value,
member identifier, access mode, and signature. Function behavior requires
separate executable reference and conformance tests.

## Relationships

- **Derived from:** MP-0001 compatibility discovery objective
- **Depends on:** VBA constant-expression and enumeration support
- **Conflicts with:** None known

## Tailoring

A member or value may be deferred only through an approved compatibility-scope
change that identifies the resulting source or run-time limitation.

## Implementation Record

Not implemented.

# REQ-0098 — stdole constants contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed OLE Automation standard type library (`stdole2.tlb`)

## Scope

This requirement applies to all constants exposed by the standard OLE library.

## Requirement

WFC shall expose the following enumeration names, member names, integral
values, and source-visible types.

### `OLE_TRISTATE` (`{6650430A-BE0F-101A-8BBB-00AA00300CAB}`)

| Name | Value | Type | Member ID |
| --- | ---: | --- | ---: |
| `Unchecked` | 0 | `Integer` | 1073741824 |
| `Checked` | 1 | `Integer` | 1073741825 |
| `Gray` | 2 | `Integer` | 1073741826 |

### `LoadPictureConstants` (`{E6C8FA08-BD9F-11D0-985E-00C04FC29993}`)

| Name | Value | Type | Member ID |
| --- | ---: | --- | ---: |
| `Default` | 0 | `Integer` | 1073741824 |
| `Monochrome` | 1 | `Integer` | 1073741825 |
| `VgaColor` | 2 | `Integer` | 1073741826 |
| `Color` | 4 | `Integer` | 1073741827 |

## Rationale

VB6 forms and intrinsic controls exchange standard OLE font and picture
objects. Preserving these contracts is required for source, Automation,
persistence, and binary interoperability.

## Verification

**Method:** Inspection and test

**References:** `evidence/reference/stdole-2.0-type-library.json`; test case TBD

Compile representative VB6 source against the WFC declarations and compare each integral value with the retained inventory.

## Relationships

- **Derived from:** MP-0001 compatibility discovery objective
- **Depends on:** COM and OLE Automation compatibility architecture
- **Conflicts with:** None known

## Tailoring

A listed interface or member may be deferred only through an approved
compatibility-scope change identifying the affected VB6 features.

## Implementation Record

Not implemented.

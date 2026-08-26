# REQ-0096 — stdole StdPicture API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed OLE Automation standard type library (`stdole2.tlb`)

## Scope

This requirement applies to the WFC implementation of `StdPicture` on supported Windows targets.

## Requirement

WFC shall provide the `StdPicture` coclass with the reference CLSID,
ProgID, Automation interface, native COM interface, event interface where
present, member ordering, member identifiers, and signatures listed below.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `stdole` 2.0, `{00020430-0000-0000-C000-000000000046}` |
| CLSID | `{0BE35204-8F91-11CE-9DE3-00AA004BB851}` |
| ProgID | `StdPicture` |

## Interface `IPicture` (`{7BF80980-BF32-101A-8BBB-00AA00300CAB}`)

| Member | Kind | Parameters | Returns | Member ID |
| --- | --- | --- | --- | ---: |
| `Handle` | PropertyGet | `` | `Integer` | 1610678272 |
| `hPal` | PropertyGet | `` | `Integer` | 1610678273 |
| `Type` | PropertyGet | `` | `Integer` | 1610678274 |
| `Width` | PropertyGet | `` | `Long` | 1610678275 |
| `Height` | PropertyGet | `` | `Long` | 1610678276 |
| `Render` | Method | `hdc: Integer, x: Long, y: Long, cx: Long, cy: Long, xSrc: Long, ySrc: Long, cxSrc: Long, cySrc: Long, prcWBounds: Void` | `HRESULT` | 1610678277 |
| `hPal` | PropertyLet | `` | `Integer` | 1610678273 |
| `CurDC` | PropertyGet | `` | `Integer` | 1610678279 |
| `SelectPicture` | Method | `hdcIn: Integer, out phdcOut: Integer, out phbmpOut: Integer` | `HRESULT` | 1610678280 |
| `KeepOriginalFormat` | PropertyGet | `` | `Boolean` | 1610678281 |
| `KeepOriginalFormat` | PropertyLet | `` | `Boolean` | 1610678281 |
| `PictureChanged` | Method | `` | `HRESULT` | 1610678283 |
| `SaveAsFile` | Method | `pstm: Void, fSaveMemCopy: Boolean, out pcbSize: Long` | `HRESULT` | 1610678284 |
| `Attributes` | PropertyGet | `` | `Long` | 1610678285 |
| `SetHdc` | Method | `hdc: Integer` | `HRESULT` | 1610678286 |

## Interface `Picture` (`{7BF80981-BF32-101A-8BBB-00AA00300CAB}`)

| Member | Kind | Parameters | Returns | Member ID |
| --- | --- | --- | --- | ---: |
| `hPal` | Member | `` | `Integer` | 2 |
| `Render` | Method | `hdc: Integer, x: Long, y: Long, cx: Long, cy: Long, xSrc: Long, ySrc: Long, cxSrc: Long, cySrc: Long, prcWBounds: Void` | `Void` | 6 |

## Rationale

VB6 forms and intrinsic controls exchange standard OLE font and picture
objects. Preserving these contracts is required for source, Automation,
persistence, and binary interoperability.

## Verification

**Method:** Inspection and test

**References:** `evidence/reference/stdole-2.0-type-library.json`; test case TBD

Compare the emitted type information and vtable layout with the retained inventory, then exercise object creation, property mutation, notification, persistence, and rendering in reference and WFC tests.

## Relationships

- **Derived from:** MP-0001 compatibility discovery objective
- **Depends on:** COM and OLE Automation compatibility architecture
- **Conflicts with:** None known

## Tailoring

A listed interface or member may be deferred only through an approved
compatibility-scope change identifying the affected VB6 features.

## Implementation Record

Not implemented.

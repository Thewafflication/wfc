# REQ-0095 — stdole StdFont API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed OLE Automation standard type library (`stdole2.tlb`)

## Scope

This requirement applies to the WFC implementation of `StdFont` on supported Windows targets.

## Requirement

WFC shall provide the `StdFont` coclass with the reference CLSID,
ProgID, Automation interface, native COM interface, event interface where
present, member ordering, member identifiers, and signatures listed below.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `stdole` 2.0, `{00020430-0000-0000-C000-000000000046}` |
| CLSID | `{0BE35203-8F91-11CE-9DE3-00AA004BB851}` |
| ProgID | `StdFont` |

## Interface `IFont` (`{BEF6E002-A874-101A-8BBA-00AA00300CAB}`)

| Member | Kind | Parameters | Returns | Member ID |
| --- | --- | --- | --- | ---: |
| `Name` | PropertyGet | `` | `String` | 1610678272 |
| `Name` | PropertyLet | `` | `String` | 1610678272 |
| `Size` | PropertyGet | `` | `Currency` | 1610678274 |
| `Size` | PropertyLet | `` | `Currency` | 1610678274 |
| `Bold` | PropertyGet | `` | `Boolean` | 1610678276 |
| `Bold` | PropertyLet | `` | `Boolean` | 1610678276 |
| `Italic` | PropertyGet | `` | `Boolean` | 1610678278 |
| `Italic` | PropertyLet | `` | `Boolean` | 1610678278 |
| `Underline` | PropertyGet | `` | `Boolean` | 1610678280 |
| `Underline` | PropertyLet | `` | `Boolean` | 1610678280 |
| `Strikethrough` | PropertyGet | `` | `Boolean` | 1610678282 |
| `Strikethrough` | PropertyLet | `` | `Boolean` | 1610678282 |
| `Weight` | PropertyGet | `` | `Integer` | 1610678284 |
| `Weight` | PropertyLet | `` | `Integer` | 1610678284 |
| `Charset` | PropertyGet | `` | `Integer` | 1610678286 |
| `Charset` | PropertyLet | `` | `Integer` | 1610678286 |
| `hFont` | PropertyGet | `` | `Integer` | 1610678288 |
| `Clone` | Method | `out ppfont: IFont` | `HRESULT` | 1610678289 |
| `IsEqual` | Method | `pfontOther: IFont` | `HRESULT` | 1610678290 |
| `SetRatio` | Method | `cyLogical: Long, cyHimetric: Long` | `HRESULT` | 1610678291 |
| `AddRefHfont` | Method | `hFont: Integer` | `HRESULT` | 1610678292 |
| `ReleaseHfont` | Method | `hFont: Integer` | `HRESULT` | 1610678293 |

## Interface `Font` (`{BEF6E003-A874-101A-8BBA-00AA00300CAB}`)

| Member | Kind | Parameters | Returns | Member ID |
| --- | --- | --- | --- | ---: |
| `Name` | Member | `` | `String` | 0 |
| `Size` | Member | `` | `Currency` | 2 |
| `Bold` | Member | `` | `Boolean` | 3 |
| `Italic` | Member | `` | `Boolean` | 4 |
| `Underline` | Member | `` | `Boolean` | 5 |
| `Strikethrough` | Member | `` | `Boolean` | 6 |
| `Weight` | Member | `` | `Integer` | 7 |
| `Charset` | Member | `` | `Integer` | 8 |

## Event interface `FontEvents` (`{4EF6100A-AF88-11D0-9846-00C04FC29993}`)

| Member | Kind | Parameters | Returns | Member ID |
| --- | --- | --- | --- | ---: |
| `FontChanged` | Method | `PropertyName: String` | `Void` | 9 |

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

# REQ-0058 — VB App API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `App` class on supported Windows targets.

## Requirement

The WFC `App` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD4F78-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_App`, `{33AD4F79-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | None exposed |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `Comments` | Get | `String` | 65553 | — |
| `CompanyName` | Get | `String` | 65554 | — |
| `EXEName` | Get | `String` | 65538 | — |
| `FileDescription` | Get | `String` | 65555 | — |
| `HelpFile` | Get/Let | `String` | 65565 | — |
| `hInstance` | Get | `Long` | 65559 | — |
| `LegalCopyright` | Get | `String` | 65556 | — |
| `LegalTrademarks` | Get | `String` | 65557 | — |
| `LogMode` | Get | `Long` | 65562 | — |
| `LogPath` | Get | `String` | 65561 | — |
| `Major` | Get | `Integer` | 65550 | — |
| `Minor` | Get | `Integer` | 65551 | — |
| `NonModalAllowed` | Get | `Boolean` | 65560 | — |
| `OleRequestPendingMsgText` | Get/Let | `String` | 65549 | — |
| `OleRequestPendingMsgTitle` | Get/Let | `String` | 65548 | — |
| `OleRequestPendingTimeout` | Get/Let | `Long` | 65547 | — |
| `OleServerBusyMsgText` | Get/Let | `String` | 65545 | — |
| `OleServerBusyMsgTitle` | Get/Let | `String` | 65544 | — |
| `OleServerBusyRaiseError` | Get/Let | `Boolean` | 65546 | — |
| `OleServerBusyTimeout` | Get/Let | `Long` | 65543 | — |
| `Path` | Get | `String` | 65537 | — |
| `PrevInstance` | Get | `Boolean` | 65540 | — |
| `ProductName` | Get | `String` | 65558 | — |
| `RetainedProject` | Get | `Boolean` | 65566 | — |
| `Revision` | Get | `Integer` | 65552 | — |
| `StartMode` | Get | `Integer` | 65541 | — |
| `TaskVisible` | Get/Let | `Boolean` | 65542 | — |
| `ThreadID` | Get | `Long` | 65564 | — |
| `Title` | Get/Let | `String` | 65539 | — |
| `UnattendedApp` | Get | `Boolean` | 65563 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `StartLogging` | `LogTarget: String, LogModes: Long` | `HRESULT` | 131072 | — |
| `LogEvent` | `LogBuffer: String, EventType: Variant optional` | `HRESULT` | 131073 | — |

## Event Inventory

The reference class exposes no members on its default event interface.

## Rationale

The VB object library defines the source-visible form, intrinsic-control, and
run-time service classes required to load and execute VB6 form projects.

## Verification

**Method:** Inspection

**References:** `evidence/reference/vb-6.0-type-library.json`; test case TBD

Inspect the WFC type information and compare every listed member name, DISPID,
access mode, signature, and event. Behavioral compatibility requires separate
requirements and executable VB6 reference tests.

## Relationships

- **Derived from:** MP-0001 compatibility discovery objective
- **Depends on:** WCRT integration and form-runtime architecture decisions
- **Conflicts with:** None known

## Tailoring

A member may be deferred only through an approved compatibility-scope change
that identifies the resulting form or source-compatibility limitation.

## Implementation Record

Not implemented.

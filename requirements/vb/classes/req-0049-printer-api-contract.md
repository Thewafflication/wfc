# REQ-0049 — VB Printer API contract

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed Visual Basic 6.0 object library (`VB6.OLB`)

## Scope

This requirement applies to the WFC compatibility implementation of the `Printer` class on supported Windows targets.

## Requirement

The WFC `Printer` class shall expose the properties, methods, and events
listed below with the specified Automation names, member identifiers, access
modes, and parameter and return types. Hidden and non-browsable listed members
are part of the compatibility contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `VB` 6.0, `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Reference CLSID | `{33AD4F30-6699-11CF-B70C-00AA0060D393}` |
| Default interface | `_Printer`, `{33AD4F31-6699-11CF-B70C-00AA0060D393}` |
| Default event interface | None exposed |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `ColorMode` | Get/Let | `Integer` | 65572 | — |
| `Copies` | Get/Let | `Integer` | 65570 | — |
| `CurrentX` | Get/Let | `Single` | 65546 | — |
| `CurrentY` | Get/Let | `Single` | 65547 | — |
| `DeviceName` | Get | `String` | 65564 | — |
| `DrawMode` | Get/Let | `Integer` | 65558 | — |
| `DrawStyle` | Get/Let | `Integer` | 65554 | — |
| `DrawWidth` | Get/Let | `Integer` | 65555 | — |
| `DriverName` | Get | `String` | 65565 | — |
| `Duplex` | Get/Let | `Integer` | 65573 | — |
| `FillColor` | Get/Let | `Long` | 65557 | — |
| `FillStyle` | Get/Let | `Integer` | 65556 | — |
| `Font` | Get/Set | `Font` | 65576 | — |
| `FontBold` | Get/Let | `Boolean` | 65541 | — |
| `FontCount` | Get | `Integer` | 65560 | — |
| `FontItalic` | Get/Let | `Boolean` | 65542 | — |
| `FontName` | Get/Let | `String` | 65539 | — |
| `Fonts(: Integer)` | Get | `String` | 65561 | — |
| `FontSize` | Get/Let | `Single` | 65540 | — |
| `FontStrikethru` | Get/Let | `Boolean` | 65543 | — |
| `FontTransparent` | Get/Let | `Boolean` | 65553 | — |
| `FontUnderline` | Get/Let | `Boolean` | 65544 | — |
| `ForeColor` | Get/Let | `Long` | 65578 | — |
| `hDC` | Get | `Long` | 65545 | — |
| `Height` | Get/Let | `Long` | 65538 | — |
| `Orientation` | Get/Let | `Integer` | 65567 | — |
| `Page` | Get | `Integer` | 65559 | — |
| `PaperBin` | Get/Let | `Integer` | 65571 | — |
| `PaperSize` | Get/Let | `Integer` | 65568 | — |
| `Port` | Get | `String` | 65566 | — |
| `PrintQuality` | Get/Let | `Integer` | 65575 | — |
| `RightToLeft` | Get/Let | `Boolean` | 65577 | — |
| `ScaleHeight` | Get/Let | `Single` | 65551 | — |
| `ScaleLeft` | Get/Let | `Single` | 65548 | — |
| `ScaleMode` | Get/Let | `Integer` | 65552 | — |
| `ScaleTop` | Get/Let | `Single` | 65549 | — |
| `ScaleWidth` | Get/Let | `Single` | 65550 | — |
| `TrackDefault` | Get/Let | `Boolean` | 65574 | — |
| `TwipsPerPixelX` | Get | `Single` | 65562 | — |
| `TwipsPerPixelY` | Get | `Single` | 65563 | — |
| `Width` | Get/Let | `Long` | 65537 | — |
| `Zoom` | Get/Let | `Long` | 65569 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `EndDoc` | `` | `Void` | 131072 | — |
| `NewPage` | `` | `Void` | 131073 | — |
| `KillDoc` | `` | `Void` | 131074 | — |
| `Circle` | `Step: Integer, X: Single, Y: Single, Radius: Single, Color: Long, Start: Single, End: Single, Aspect: Single` | `Void` | 131075 | — |
| `Line` | `Flags: Integer, X1: Single, Y1: Single, X2: Single, Y2: Single, Color: Long` | `Void` | 131076 | — |
| `PaintPicture` | `Picture: Picture, X1: Single, Y1: Single, Width1: Variant optional, Height1: Variant optional, X2: Variant optional, Y2: Variant optional, Width2: Variant optional, Height2: Variant optional, Opcode: Variant optional` | `Void` | 131077 | — |
| `PSet` | `Step: Integer, X: Single, Y: Single, Color: Long` | `Void` | 131078 | — |
| `Scale` | `Flags: Integer, X1: Variant optional, Y1: Variant optional, X2: Variant optional, Y2: Variant optional` | `Void` | 131079 | — |
| `ScaleX` | `Width: Single, FromScale: Variant optional, ToScale: Variant optional` | `Single` | 131080 | — |
| `ScaleY` | `Height: Single, FromScale: Variant optional, ToScale: Variant optional` | `Single` | 131081 | — |
| `TextWidth` | `Str: String` | `Single` | 131082 | — |
| `TextHeight` | `Str: String` | `Single` | 131083 | — |

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

# REQ-0006 — ListView Automation API contract

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Installed MSComctlLib 2.0 type library; MP-0001 discovery baseline

## Scope

This requirement applies to the WFC compatibility implementation of the
`ListView` ActiveX control on Windows x86, x64, and ARM64. The
inventory below describes the reference x86 Automation contract. Reference
GUIDs identify the observed component but do not require WFC to reuse them;
binary identity and registration coexistence require a separate ADR.

## Requirement

The WFC `ListView` compatibility component shall expose the
properties, methods, and events listed in this document with the specified
Automation names, member identifiers, access modes, and parameter and return
types. Hidden and non-browsable listed members are part of this contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `MSComctlLib` 2.0, `{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}` |
| ProgID | `MSComctlLib.ListViewCtrl.2` |
| Reference CLSID | `{BDD1F04B-858B-11D1-B16A-00C0F0283628}` |
| Default interface | `IListView`, `{BDD1F049-858B-11D1-B16A-00C0F0283628}` |
| Default event interface | `ListViewEvents`, `{BDD1F04A-858B-11D1-B16A-00C0F0283628}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `AllowColumnReorder` | Get/Let | `Boolean` | 23 | — |
| `Appearance` | Get/Let | `AppearanceConstants` | -520 | — |
| `Arrange` | Get/Let | `ListArrangeConstants` | 1 | — |
| `BackColor` | Get/Let | `ULong` | -501 | — |
| `BorderStyle` | Get/Let | `BorderStyleConstants` | -504 | — |
| `Checkboxes` | Get/Let | `Boolean` | 24 | — |
| `ColumnHeaderIcons` | Get/Set/Let | `Object` | 32 | — |
| `ColumnHeaders` | Get/Let | `IColumnHeaders` | 2 | — |
| `DropHighlight` | Get/Set/Let | `IListItem` | 3 | NonBrowsable |
| `Enabled` | Get/Let | `Boolean` | -514 | — |
| `FlatScrollBar` | Get/Let | `Boolean` | 25 | — |
| `Font` | Get/Set | `Font` | -512 | — |
| `ForeColor` | Get/Let | `ULong` | -513 | — |
| `FullRowSelect` | Get/Let | `Boolean` | 26 | — |
| `GridLines` | Get/Let | `Boolean` | 27 | — |
| `HideColumnHeaders` | Get/Let | `Boolean` | 4 | — |
| `HideSelection` | Get/Let | `Boolean` | 5 | — |
| `HotTracking` | Get/Let | `Boolean` | 28 | — |
| `HoverSelection` | Get/Let | `Boolean` | 29 | — |
| `hWnd` | Get/Let | `Integer` | -515 | NonBrowsable |
| `Icons` | Get/Set/Let | `Object` | 6 | — |
| `LabelEdit` | Get/Let | `ListLabelEditConstants` | 8 | — |
| `LabelWrap` | Get/Let | `Boolean` | 9 | — |
| `ListItems` | Get/Let | `IListItems` | 7 | NonBrowsable |
| `MouseIcon` | Get/Let/Set | `Picture` | 10 | — |
| `MousePointer` | Get/Let | `MousePointerConstants` | 11 | — |
| `MultiSelect` | Get/Let | `Boolean` | 12 | — |
| `OLEDragMode` | Get/Let | `OLEDragConstants` | 1550 | — |
| `OLEDropMode` | Get/Let | `OLEDropConstants` | 1551 | — |
| `Picture` | Get/Let/Set | `Picture` | 31 | — |
| `PictureAlignment` | Get/Let | `ListPictureAlignmentConstants` | 30 | — |
| `SelectedItem` | Get/Set/Let | `IListItem` | 13 | NonBrowsable |
| `SmallIcons` | Get/Set/Let | `Object` | 14 | — |
| `Sorted` | Get/Let | `Boolean` | 15 | — |
| `SortKey` | Get/Let | `Integer` | 16 | — |
| `SortOrder` | Get/Let | `ListSortOrderConstants` | 17 | — |
| `TextBackground` | Get/Let | `ListTextBackgroundConstants` | 33 | — |
| `View` | Get/Let | `ListViewConstants` | 18 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `FindItem` | `sz: String, Where: Variant optional, Index: Variant optional, fPartial: Variant optional` | `IListItem` | 19 | — |
| `GetFirstVisible` | `` | `IListItem` | 20 | — |
| `HitTest` | `x: Single, y: Single` | `IListItem` | 21 | NonBrowsable |
| `StartLabelEdit` | `` | `Void` | 22 | — |
| `OLEDrag` | `` | `Void` | 1552 | — |
| `Refresh` | `` | `Void` | -550 | — |
| `AboutBox` | `` | `Void` | -552 | Hidden |

## Event Inventory

| Event | Parameters | DISPID |
| --- | --- | ---: |
| `BeforeLabelEdit` | `Cancel: Integer` | 1 |
| `AfterLabelEdit` | `Cancel: Integer, NewString: String` | 2 |
| `ColumnClick` | `ColumnHeader: ColumnHeader` | 3 |
| `ItemClick` | `Item: ListItem` | 4 |
| `KeyDown` | `KeyCode: Integer, Shift: Integer` | -602 |
| `KeyUp` | `KeyCode: Integer, Shift: Integer` | -604 |
| `KeyPress` | `KeyAscii: Integer` | -603 |
| `MouseDown` | `Button: Integer, Shift: Integer, x: Long, y: Long` | -605 |
| `MouseMove` | `Button: Integer, Shift: Integer, x: Long, y: Long` | -606 |
| `MouseUp` | `Button: Integer, Shift: Integer, x: Long, y: Long` | -607 |
| `Click` | `` | -600 |
| `DblClick` | `` | -601 |
| `OLEStartDrag` | `ByRef Data: DataObject, ByRef AllowedEffects: Long` | 1550 |
| `OLEGiveFeedback` | `ByRef Effect: Long, ByRef DefaultCursors: Boolean` | 1551 |
| `OLESetData` | `ByRef Data: DataObject, ByRef DataFormat: Integer` | 1552 |
| `OLECompleteDrag` | `ByRef Effect: Long` | 1553 |
| `OLEDragOver` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef x: Single, ByRef y: Single, ByRef State: Integer` | 1554 |
| `OLEDragDrop` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef x: Single, ByRef y: Single` | 1555 |
| `ItemCheck` | `Item: ListItem` | 5 |

## Behavioral Characterization Needed

The type library establishes structure, not runtime semantics. Follow-on
behavioral requirements and VB6 reference probes shall cover:

- List item, subitem, and column-header collection management.
- Icon, small-icon, list, and report views with sorting, selection, checks, and label editing.
- Search, hit testing, scrolling, images, rendering options, and event ordering.
- Default values, boundary values, invalid inputs, error numbers, persistence,
  rendering, focus, keyboard navigation, and behavior at common DPI settings.

## Rationale

Matching the published Automation surface is necessary for source-level VB6
compatibility and provides a stable boundary for later behavioral probes.

## Verification

**Method:** Inspection

**References:** `evidence/reference/mscomctl-2.0-type-library.json`; test case TBD

Inspect the WFC type library and compare every listed member name, DISPID,
access mode, signature, and event against this requirement. Runtime semantics
are verified by separate behavioral requirements and tests.

## Relationships

- **Derived from:** MP-0001 compatibility discovery objective
- **Depends on:** WCRT integration ADR and COM identity/coexistence ADR
- **Conflicts with:** None known

## Tailoring

A member may be deferred only through an approved compatibility-scope change
that identifies the resulting limitation.

## Implementation Record

Not implemented.

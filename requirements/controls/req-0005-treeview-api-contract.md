# REQ-0005 — TreeView Automation API contract

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Installed MSComctlLib 2.0 type library; MP-0001 discovery baseline

## Scope

This requirement applies to the WFC compatibility implementation of the
`TreeView` ActiveX control on Windows x86, x64, and ARM64. The
inventory below describes the reference x86 Automation contract. Reference
GUIDs identify the observed component but do not require WFC to reuse them;
binary identity and registration coexistence require a separate ADR.

## Requirement

The WFC `TreeView` compatibility component shall expose the
properties, methods, and events listed in this document with the specified
Automation names, member identifiers, access modes, and parameter and return
types. Hidden and non-browsable listed members are part of this contract.

## Reference Identity

| Item | Observed value |
| --- | --- |
| Type library | `MSComctlLib` 2.0, `{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}` |
| ProgID | `MSComctlLib.TreeCtrl.2` |
| Reference CLSID | `{C74190B6-8589-11D1-B16A-00C0F0283628}` |
| Default interface | `ITreeView`, `{C74190B4-8589-11D1-B16A-00C0F0283628}` |
| Default event interface | `ITreeViewEvents`, `{C74190B5-8589-11D1-B16A-00C0F0283628}` |

## Property Inventory

| Property | Access | Type | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `Appearance` | Get/Let | `AppearanceConstants` | -520 | — |
| `BorderStyle` | Get/Let | `BorderStyleConstants` | -504 | — |
| `Checkboxes` | Get/Let | `Boolean` | 17 | — |
| `DropHighlight` | Get/Set/Let | `INode` | 1 | NonBrowsable |
| `Enabled` | Get/Let | `Boolean` | -514 | — |
| `Font` | Get/Let/Set | `Font` | -512 | — |
| `FullRowSelect` | Get/Let | `Boolean` | 18 | — |
| `HideSelection` | Get/Let | `Boolean` | 2 | — |
| `HotTracking` | Get/Let | `Boolean` | 19 | — |
| `hWnd` | Get/Let | `Integer` | -515 | NonBrowsable |
| `ImageList` | Get/Set/Let | `Object` | 3 | — |
| `Indentation` | Get/Let | `Single` | 4 | — |
| `LabelEdit` | Get/Let | `LabelEditConstants` | 5 | — |
| `LineStyle` | Get/Let | `TreeLineStyleConstants` | 6 | — |
| `MouseIcon` | Get/Let/Set | `Picture` | 8 | — |
| `MousePointer` | Get/Let | `MousePointerConstants` | 7 | — |
| `Nodes` | Get/Set | `INodes` | 9 | NonBrowsable |
| `OLEDragMode` | Get/Let | `OLEDragConstants` | 1550 | — |
| `OLEDropMode` | Get/Let | `OLEDropConstants` | 1551 | — |
| `PathSeparator` | Get/Let | `String` | 10 | — |
| `Scroll` | Get/Let | `Boolean` | 20 | — |
| `SelectedItem` | Get/Set/Let | `INode` | 11 | NonBrowsable |
| `SingleSel` | Get/Let | `Boolean` | 21 | — |
| `Sorted` | Get/Let | `Boolean` | 12 | — |
| `Style` | Get/Let | `TreeStyleConstants` | 13 | — |

## Method Inventory

| Method | Parameters | Returns | DISPID | Flags |
| --- | --- | --- | ---: | --- |
| `HitTest` | `x: Single, y: Single` | `INode` | 14 | NonBrowsable |
| `GetVisibleCount` | `` | `Long` | 15 | NonBrowsable |
| `StartLabelEdit` | `` | `Void` | 16 | — |
| `Refresh` | `` | `Void` | -550 | — |
| `AboutBox` | `` | `Void` | -552 | Hidden |
| `OLEDrag` | `` | `Void` | 1552 | — |

## Event Inventory

| Event | Parameters | DISPID |
| --- | --- | ---: |
| `BeforeLabelEdit` | `Cancel: Integer` | 1 |
| `AfterLabelEdit` | `Cancel: Integer, NewString: String` | 2 |
| `Collapse` | `Node: Node` | 3 |
| `Expand` | `Node: Node` | 4 |
| `NodeClick` | `Node: Node` | 5 |
| `KeyDown` | `KeyCode: Integer, Shift: Integer` | -602 |
| `KeyUp` | `KeyCode: Integer, Shift: Integer` | -604 |
| `KeyPress` | `KeyAscii: Integer` | -603 |
| `MouseDown` | `Button: Integer, Shift: Integer, x: Long, y: Long` | -605 |
| `MouseMove` | `Button: Integer, Shift: Integer, x: Long, y: Long` | -606 |
| `MouseUp` | `Button: Integer, Shift: Integer, x: Long, y: Long` | -607 |
| `Click` | `` | -600 |
| `DblClick` | `` | -601 |
| `NodeCheck` | `Node: Node` | 6 |
| `OLEStartDrag` | `ByRef Data: DataObject, ByRef AllowedEffects: Long` | 1550 |
| `OLEGiveFeedback` | `ByRef Effect: Long, ByRef DefaultCursors: Boolean` | 1551 |
| `OLESetData` | `ByRef Data: DataObject, ByRef DataFormat: Integer` | 1552 |
| `OLECompleteDrag` | `ByRef Effect: Long` | 1553 |
| `OLEDragOver` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef x: Single, ByRef y: Single, ByRef State: Integer` | 1554 |
| `OLEDragDrop` | `ByRef Data: DataObject, ByRef Effect: Long, ByRef Button: Integer, ByRef Shift: Integer, ByRef x: Single, ByRef y: Single` | 1555 |

## Behavioral Characterization Needed

The type library establishes structure, not runtime semantics. Follow-on
behavioral requirements and VB6 reference probes shall cover:

- Hierarchical node collection creation, removal, lookup, and traversal.
- Selection, expansion, checkboxes, sorting, label editing, images, and hit testing.
- Node, keyboard, pointer, label-edit, and OLE drag-and-drop event ordering.
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

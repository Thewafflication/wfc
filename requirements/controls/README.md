# MSComctlLib Control Requirements

**Content type:** Project requirement index  
**Status:** Proposed  
**Reference baseline:** `MSCOMCTL.OCX` 6.00.8177, MSComctlLib 2.0

These requirements capture the Automation API surface of every control class
marked as a control in the installed type library. Supporting collections and
value-object interfaces are referenced by these contracts and will receive
separate requirements as behavioral probes are designed.

| Requirement | Control | Reference ProgID | Properties | Methods | Events | Status |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `REQ-0001` | [TabStrip](req-0001-tabstrip-api-contract.md) | `MSComctlLib.TabStrip.2` | 25 | 4 | 14 | Proposed |
| `REQ-0002` | [Toolbar](req-0002-toolbar-api-contract.md) | `MSComctlLib.Toolbar.2` | 21 | 6 | 15 | Proposed |
| `REQ-0003` | [StatusBar](req-0003-statusbar-api-contract.md) | `MSComctlLib.SBarCtrl.2` | 11 | 3 | 13 | Proposed |
| `REQ-0004` | [ProgressBar](req-0004-progressbar-api-contract.md) | `MSComctlLib.ProgCtrl.2` | 13 | 3 | 10 | Proposed |
| `REQ-0005` | [TreeView](req-0005-treeview-api-contract.md) | `MSComctlLib.TreeCtrl.2` | 25 | 6 | 20 | Proposed |
| `REQ-0006` | [ListView](req-0006-listview-api-contract.md) | `MSComctlLib.ListViewCtrl.2` | 38 | 7 | 19 | Proposed |
| `REQ-0007` | [ImageList](req-0007-imagelist-api-contract.md) | `MSComctlLib.ImageListCtrl.2` | 7 | 2 | 0 | Proposed |
| `REQ-0008` | [Slider](req-0008-slider-api-contract.md) | `MSComctlLib.Slider.2` | 21 | 5 | 15 | Proposed |
| `REQ-0009` | [ImageCombo](req-0009-imagecombo-api-contract.md) | `MSComctlLib.ImageComboCtl.2` | 18 | 4 | 12 | Proposed |

## Evidence Boundary

The inventories are generated from `evidence/reference/mscomctl-2.0-type-library.json`.
They establish names, DISPIDs, access modes, and Automation signatures. They do
not establish default values, state transitions, rendering, event order, error
semantics, persistence, or accessibility behavior; those require executable VB6
reference probes and separately verifiable requirements.

## Behavioral Requirements

The [behavioral requirement set](behavior/README.md) defines the first 27
observable control behaviors derived from the installed Microsoft help. These
requirements cover all nine control classes and are allocated to planned
matching-number VB6 reference and WFC conformance tests.

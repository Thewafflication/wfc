# MSComctlLib Behavioral Requirements

**Content type:** Project requirement index  
**Status:** Proposed  
**Reference documentation:** `C:\Windows\Help\cmctl198.chm`  
**Reference documentation SHA-256:** `B60543DFA968658287DAF3939B44B35C2243D0252FEF7A7D4C6F5CCF13BF92D6`

These requirements translate the installed Microsoft Common Controls 6.0 help
into observable WFC obligations. They supplement the Automation API contracts
in the parent directory. Each remains proposed until its VB6 reference test
confirms the documented behavior against the recorded binary baseline.

## TabStrip

- [REQ-0010 — Collection and selection](req-0010-tabstrip-collection-selection.md)
- [REQ-0011 — Layout behavior](req-0011-tabstrip-layout.md)
- [REQ-0012 — Pre-selection cancellation](req-0012-tabstrip-before-click.md)

## Toolbar

- [REQ-0013 — Button behavior](req-0013-toolbar-buttons.md)
- [REQ-0014 — Customization](req-0014-toolbar-customization.md)
- [REQ-0015 — State persistence](req-0015-toolbar-persistence.md)

## StatusBar

- [REQ-0016 — Panel model](req-0016-statusbar-panels.md)
- [REQ-0017 — Normal and simple modes](req-0017-statusbar-modes.md)

## ProgressBar

- [REQ-0018 — Range and rendering](req-0018-progressbar-range.md)
- [REQ-0019 — Orientation and fill style](req-0019-progressbar-orientation-scrolling.md)

## TreeView

- [REQ-0020 — Hierarchy management](req-0020-treeview-hierarchy.md)
- [REQ-0021 — Keyboard navigation](req-0021-treeview-keyboard.md)
- [REQ-0022 — Label editing](req-0022-treeview-label-edit.md)
- [REQ-0023 — Visibility and hit testing](req-0023-treeview-spatial-query.md)

## ListView

- [REQ-0024 — View modes](req-0024-listview-views.md)
- [REQ-0025 — Item and column model](req-0025-listview-object-model.md)
- [REQ-0026 — Searching and sorting](req-0026-listview-search-sort.md)
- [REQ-0027 — Label editing](req-0027-listview-edit-presentation.md)
- [REQ-0034 — Report presentation options](req-0034-listview-report-presentation.md)

## ImageList

- [REQ-0028 — Collection and binding](req-0028-imagelist-collection-binding.md)
- [REQ-0029 — Rendering and composition](req-0029-imagelist-render-composition.md)

## Slider

- [REQ-0030 — Value and selection range](req-0030-slider-range-selection.md)
- [REQ-0031 — Input and event timing](req-0031-slider-input-events.md)
- [REQ-0032 — Ticks and orientation](req-0032-slider-ticks-orientation.md)

## ImageCombo

- [REQ-0033 — Collection insertion](req-0033-imagecombo-items.md)
- [REQ-0035 — Item presentation](req-0035-imagecombo-item-presentation.md)
- [REQ-0036 — Editing lock](req-0036-imagecombo-locking.md)

## Evidence and Acceptance

The help file is a specification source, not execution evidence. A requirement
becomes eligible for acceptance only after its planned `TC-NNNN` probe records
the installed VB6 control's actual behavior, resolves any difference from the
help, and verifies WFC against the resulting expected outcome.

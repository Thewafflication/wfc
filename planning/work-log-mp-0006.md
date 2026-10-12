# Work Log — MP-0006 `MSComctlLib` Compatibility

**Content type:** Work log (per `wsp/processes/work-log-template.md`)

**Milestone or work package:** MP-0006 — see [the plan](MP-0006-mscomctllib.md).

**Period:** 2026-10-12 onward.

**Starting baseline:** MP-0005 closeout.

**Author:** Claude (Overlord cross-project assistant), on behalf of the owner.

**Status:** Active

Goal-level token usage and elapsed time: Not reported.

## Increments

| Date / # | Type | Description | Evidence |
| --- | --- | --- | --- |
| 2026-10-12 #1 | Planning | MP-0005 closed; `ADR-0006` accepted; MP-0006 plan baselined | `MP-0005-closeout.md`, `MP-0006-mscomctllib.md` |
| 2026-10-12 #2 | Construction | Generated MSComctlLib tables, merged class tables, native control registration, generic collections and value objects, `ProgressBar`, `Slider`, `ImageList` (`REQ-0296`); corpus 188 | CTest 354/354 |
| 2026-10-12 #3 | Construction | `TabStrip` and `StatusBar` behavior, MSComctlLib enumeration constants, `As Library.Class` parameters (`REQ-0297`); corpus 189 | CTest 355/355 |
| 2026-10-12 #4 | Construction | `Toolbar` buttons, groups, drop-down menus, save/restore (`REQ-0298`); corpus 190 | CTest 356/356 |
| 2026-10-12 #5 | Construction | `TreeView` nodes, relationships, expansion, selection, search, label edit, keys (`REQ-0299`); corpus 191 | CTest 357/357 |
| 2026-10-12 #6 | Construction | `ListView` items, sub items, columns, views, sort, find, hit test (`REQ-0300`); corpus 192 | CTest 358/358 |
| 2026-10-12 #7 | Construction | `ImageCombo` items, selection, text, locking (`REQ-0301`); corpus 193 | CTest 359/359 |
| 2026-10-12 #8 | Construction | Persisted `BeginProperty` groups of the common controls loaded from forms; `OleObjectBlob` warning `WFC0293` (`REQ-0302`); corpus 194 | CTest 360/360 |
| 2026-10-12 #9 | Defect | Corpus cases 167–194 compared only stdout, but their output is `Debug.Print` text on stderr, so they passed with empty expectations; the runner now also compares an optional `<case>.debug` file and the 25 cases have one | CTest 360/360 |
| 2026-10-12 #10 | Construction | Win32 rendering and input for the nine controls through snapshots (`REQ-0296`); `TC-MP0006-gui-comctl` with screenshot | CTest 361/361 |
| 2026-10-12 #11 | Defect | Corpus 170 depended on an empty directory that Git does not keep (found by the new `.debug` comparison on CI); the fixture now holds a `.gitkeep` | CTest 361/361 |
| 2026-10-12 #12 | Construction | Images from `ImageList` in the Win32 rendering of tabs, buttons, nodes, list items and combo items (`REQ-0296`) | CTest 361/361 |
| 2026-10-12 #13 | Construction | Behavior refinements against REQ-0010–0036: cancelled tab `BeforeClick` still raises `Click`, sixteen-panel limit (35609), registry `SaveToolbar`/`RestoreToolbar`, label-edit start/cancel flow, tree wrap and type-ahead, slider selection rules, bound-ImageList append-only (35615), toolbar double-click, report-only grid lines; corpus 195 | CTest 362/362 |
| 2026-10-12 #14 | Closeout | Coverage matrix and proposed closeout record drafted | Not applicable |

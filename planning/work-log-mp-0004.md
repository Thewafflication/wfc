# Work Log — MP-0004 Forms and Intrinsic Controls

**Content type:** Work log (per `wsp/processes/work-log-template.md`)

**Milestone or work package:** MP-0004 — see
[the plan](MP-0004-forms-and-controls.md).

**Period:** 2026-10-11 onward.

**Starting baseline:** MP-0003 closeout.

**Author:** Claude (Overlord cross-project assistant), on behalf of the owner.

**Status:** Active

Goal-level token usage and elapsed time: Not reported.

## Increments

| Date / # | Type | Description | Evidence |
| --- | --- | --- | --- |
| 2026-10-11 #1 | Planning | MP-0004 plan and ADR-0005 (UI platform abstraction and test hosts) | `planning/MP-0004-forms-and-controls.md`, `architecture/adr-0005-ui-platform.md` |
| 2026-10-11 #2 | Construction | Stage 1–2 foundations: `UiHost` interface, headless host with virtual-clock timers, generated control/event tables (`scripts/New-UiControlTables.ps1`, drift check), `.frm`/`.frx` parser; `wfc_ui_tests` | CTest `TC-MP0004-*` |
| 2026-10-11 #3 | Construction | Stage 3 first slice: `.vbp` `Form=`/`Startup`, forms as class modules with native controls (table-driven properties), lifecycle and events, control arrays, scripted headless input (`REQ-0289`); corpus 151 | CTest 307/307 |

# MP-0006 — Behavior Requirement Coverage Matrix

**Content type:** Verification record  
**Status:** Draft  
**Milestone:** [MP-0006](MP-0006-mscomctllib.md)  
**Date:** 2026-10-12

Every behavior requirement `REQ-0010`–`REQ-0036` stays `Proposed` because no VB6
reference machine exists (see the plan's assumptions). "WFC test" means a CTest
corpus case that runs on the headless host; "GUI" means
`TC-MP0006-gui-comctl` (Win32 host, screenshot evidence). Corpus cases:
188 basic (ProgressBar, Slider, ImageList), 189 tabs and status bar, 190 toolbar,
191 tree view, 192 list view, 193 image combo, 194 persisted state,
195 refinements.

| Requirement | Subject | Evidence | Coverage |
| --- | --- | --- | --- |
| `REQ-0010` | TabStrip collection and selection | 189 | Covered |
| `REQ-0011` | TabStrip layout (`MultiRow`, widths, client area) | 189 (client area, widths); GUI | Partial: row wrapping and the overflow spin are the system control's |
| `REQ-0012` | `BeforeClick` cancellation | 189, 195 | Covered |
| `REQ-0013` | Toolbar buttons and styles | 190 | Covered |
| `REQ-0014` | Customization | 190, 195 | Covered headless; no customization dialog (event only) |
| `REQ-0015` | `SaveToolbar` / `RestoreToolbar` | 190, 195 (HKCU) | Covered |
| `REQ-0016` | StatusBar panels | 189, 195 (sixteen-panel limit) | Covered |
| `REQ-0017` | StatusBar modes | 189 | Covered |
| `REQ-0018` | ProgressBar range | 188 | Covered; chunk geometry is the system control's |
| `REQ-0019` | ProgressBar orientation and scrolling | 188; GUI | Covered as properties; rendering by the system control |
| `REQ-0020` | TreeView hierarchy | 191 | Covered |
| `REQ-0021` | TreeView keyboard | 191, 195 | Covered (wrap, type-ahead) |
| `REQ-0022` | TreeView label editing | 191, 195 | Covered with one difference (below) |
| `REQ-0023` | `HitTest`, `EnsureVisible` | 191, 195 | Covered |
| `REQ-0024` | ListView views | 192; GUI | Covered as properties; arrangement by the system control |
| `REQ-0025` | ListView object model | 192 | Covered |
| `REQ-0026` | `FindItem`, sorting | 192 | Covered |
| `REQ-0027` | ListView label editing | 192, 195 | Covered with one difference (below) |
| `REQ-0028` | ImageList collection and binding | 188, 195 | Covered |
| `REQ-0029` | ImageList drawing and overlay | 188, 195 | Partial: `Overlay` and `ExtractIcon` work, `Draw` is a no-op headless, the overlay mask composite is not rendered |
| `REQ-0030` | Slider range and selection | 188, 195 | Covered |
| `REQ-0031` | Slider input events | 188, 195 | Covered |
| `REQ-0032` | Slider ticks and orientation | 188 | Covered |
| `REQ-0033` | ImageCombo items | 193 | Covered |
| `REQ-0034` | ListView report presentation | 192, 195; GUI | Partial: hover selection is not implemented |
| `REQ-0035` | ImageCombo item presentation | 193; GUI | Covered |
| `REQ-0036` | ImageCombo locking | 193 | Covered |

## Recorded differences

- **Cancelled label edit.** `AfterLabelEdit` receives an empty string instead of
  `Null` when the user cancels, because a handler declared
  `NewString As String` would fail with a type mismatch on `Null`. The
  `Cancel` argument is the way to tell, and the text is not applied.
- **Rendering.** Layout, chunk size, themes and DPI scaling are those of the
  system common controls; WFC does not reproduce the OCX's owner drawing.
- **Persistence.** The MSCOMCTL `OleObjectBlob` stream is not read (warning
  `WFC0293`); see the deferred objectives in the plan.

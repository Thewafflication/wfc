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
| 2026-10-11 #4 | Construction | Timer events, control-array option groups, live `Forms` collection (native), KeyPreview, native `For Each` (`REQ-0289`); corpus 152 | CTest 308/308 |
| 2026-10-11 #5 | Construction | ListBox/ComboBox model (AddItem, RemoveItem, Clear, List, ItemData, ListIndex, Sorted, selection events); `UiHost` list calls (`REQ-0289`); corpus 153 | CTest 309/309 |
| 2026-10-11 #6 | Construction | Scroll bars: range-checked `Value`, `Change`/`Scroll` events, scripted drag/step (`REQ-0289`); corpus 154 | CTest 310/310 |
| 2026-10-11 #7 | Construction | Menus click path, non-focusable controls, scripted `MsgBox`/`InputBox` answers, modal `Show` (`REQ-0289`); corpus 155–156 | CTest 312/312 |
| 2026-10-11 #8 | Construction | Win32 `UiHost` (windows, standard controls, menus, timers, message loop, `set_value`/`set_scroll`), `--gui` option, Win32 integration and real-window tests (`REQ-0289`) | CTest `TC-MP0004-win32-host`, `-gui-form-timer` |
| 2026-10-11 #9 | Construction | Drawing surfaces: scale modes, `PSet`/`Line`/`Circle`/`Cls`/`Scale`/`Print` on forms and picture boxes, `UiHost::draw`/`measure_text`, headless drawing trace (`REQ-0290`); corpus 157 | CTest 314/314 |
| 2026-10-11 #10 | Construction | Win32 host GDI drawing: persistent surfaces for forms and picture boxes, all draw operations, text measurement (`REQ-0290`) | CTest `TC-MP0004-win32-host` |
| 2026-10-11 #11 | Construction | Keyboard navigation (Tab order, Enter/Esc default and cancel buttons, accelerators, Space), `Validate`/`CausesValidation`, scripted `press` (`REQ-0289`); corpus 158 | CTest 315/315 |
| 2026-10-11 #12 | Construction | `Controls` collection, `Parent`/`Container`/`ActiveControl`, dynamic `Load`/`Unload Control(i)`, `As Control/Form/TextBox` types, `TypeOf`/`TypeName` for controls (`REQ-0289`); corpus 159 | CTest 317/317 |
| 2026-10-11 #13 | Construction | `End` in handlers, `Control.Font` view, form default size, `ValidateControls`/`PopupMenu`/Link*/no-op methods, `.frm` font weight fix (`REQ-0289`); corpus 160–161 | CTest 319/319 |
| 2026-10-11 #14 | Construction | `StdPicture`, `LoadPicture`/`SavePicture`, Picture properties and display, `.frx` pictures, Win32 `OleLoadPicture` rendering, `App.Path` (`REQ-0291`); corpus 162 | CTest 320/320 |
| 2026-10-11 #15 | Construction | Image, Shape and Line lightweight controls drawn on their container surface, GDI shapes (`REQ-0291`); corpus 163 | CTest 321/321 |
| 2026-10-11 #16 | Construction | `--screenshots` PNG capture (Win32 host), showcase form test, README section "Run and Test a Form Project", z-order and container-parent window creation, client-sized form windows, combo text fix (`REQ-0289`) | CTest `TC-MP0004-gui-showcase` |
| 2026-10-11 #17 | Construction | Win32 `MsgBox` and `InputBox` dialogs through `UiHost::message_box`/`input_box`, tested with a helper thread (`REQ-0289`) | CTest `TC-MP0004-win32-host` |
| 2026-10-11 #18 | Construction | `SendKeys` (parser, per-focus delivery, headless typing default, Win32 `SendInput`); `type` uses key events (`REQ-0289`); corpus 164 | CTest 322/322 |
| 2026-10-11 #19 | Construction | `Point` (headless software raster, Win32 `GetPixel`) and `PaintPicture` with source rectangles (`REQ-0290`); corpus 165 | CTest 323/323 |
| 2026-10-11 #20 | Construction | `FillColor`/`FillStyle` for `Circle` and boxes (`REQ-0290`); corpus 166 | CTest 324/324 |
| 2026-10-11 #21 | Construction | MDI forms: `MDIChild`, `ActiveForm`, `Arrange`, ordered child unload with `UnloadMode` 4, Win32 frame/`MDICLIENT`/`WS_EX_MDICHILD`, MDI screenshot composition (`REQ-0289`); corpus 167 | CTest `TC-MP0004-gui-mdi` |
| 2026-10-11 #22 | Construction | Manual drag and drop: `Drag` method, scripted `drag` input raising `DragOver`/`DragDrop`, drag constants (`REQ-0289`); corpus 168 | CTest 328/328 |

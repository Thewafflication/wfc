# Work Log — MP-0005 Component Authoring, `stdole`, and OLE2

**Content type:** Work log (per `wsp/processes/work-log-template.md`)

**Milestone or work package:** MP-0005 — see
[the plan](MP-0005-component-authoring.md).

**Period:** 2026-10-11 onward.

**Starting baseline:** MP-0004 closeout.

**Author:** Claude (Overlord cross-project assistant), on behalf of the owner.

**Status:** Active

Goal-level token usage and elapsed time: Not reported.

## Increments

| Date / # | Type | Description | Evidence |
| --- | --- | --- | --- |
| 2026-10-11 #1 | Planning | MP-0004 closed; MP-0005 plan baselined | `MP-0004-closeout.md`, `MP-0005-component-authoring.md` |
| 2026-10-11 #2 | Construction | `.frx` list items, `Controls.Add`/`Remove`, `WithEvents` on intrinsic controls (`REQ-0289`); corpus 172, 173 | CTest 335/335 |
| 2026-10-11 #3 | Construction | Graphical button pictures (`Style = 1`) on the Win32 host and headless trace (`REQ-0289`); corpus 174 | CTest 336/336 |
| 2026-10-11 #4 | Construction | Standalone `StdFont` (Clone, IsEqual, hFont, FontChanged, Weight) and `StdPicture` extras (`REQ-0095`, `REQ-0096`, `REQ-0291`); corpus 175 | CTest 337/337 |
| 2026-10-11 #5 | Construction | `PropertyBag`; `write_frm_header` and corpus round trip (`REQ-0292`, `REQ-0115`); corpus 176 | CTest 338/338 |
| 2026-10-11 #6 | Construction | ActiveX control hosting: `Object=` lines, ATL container (Win32) and unsited creation (headless), extender, event wiring, `ADR-0006` draft (`REQ-0293`); corpus 177, `TC-MP0005-gui-activex` | CTest 340/340 |
| 2026-10-11 #7 | Construction | `Controls.Add` and control arrays for ActiveX controls (`REQ-0293`); corpus 178, 179 | CTest 342/342 |
| 2026-10-11 #8 | Construction | In-project `UserControl` (`.ctl`) hosted on forms: extender, public members, events, lifecycle, `Form_Initialize` (`REQ-0294`); corpus 180, `TC-MP0005-gui-usercontrol` | CTest 344/344 |
| 2026-10-11 #9 | Construction | `Ambient`, `PropertyChanged`, `As VBControlExtender` (`REQ-0293`, `REQ-0294`) | CTest 344/344 |
| 2026-10-12 #10 | Construction | COM server: exposed classes, dynamic `IDispatch`, class factories, `--serve`/`--server`/`--register`/`--unregister` (`REQ-0295`, `ADR-0004`); in-process and two-process tests | CTest 346/346 |
| 2026-10-12 #11 | Construction | `Licenses`; mouse events and real mouse drag in the Win32 host, scripted `mouse*` input (`REQ-0289`); corpus 183, 184, Win32 host test | CTest 348/348 |
| 2026-10-12 #12 | Construction | `DataObject`, OLE drag constants, headless OLE drag sequence (`REQ-0289`); corpus 185 | CTest 349/349 |
| 2026-10-12 #13 | Construction | OLE container control shell: `CreateEmbed`, `Object`, `Close`/`Delete` (`REQ-0293`, `REQ-0063`); corpus 186 | CTest 350/350 |
| 2026-10-12 #14 | Construction | COM server type library export, `IConnectionPointContainer`, `IProvideClassInfo2`, events to clients, `--write-typelib` (`REQ-0295`); `TC-MP0005-com-server-early` | CTest 351/351 |

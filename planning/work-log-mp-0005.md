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

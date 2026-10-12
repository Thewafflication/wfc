# Work Log — MP-0008 Project Compatibility

**Content type:** Work log (per `wsp/processes/work-log-template.md`)

**Milestone or work package:** MP-0008 — see [the plan](MP-0008-project-compatibility.md).

**Period:** 2026-10-12 onward.

**Starting baseline:** MP-0007 closeout.

**Author:** Claude (Overlord cross-project assistant), on behalf of the owner.

| Date / # | Type | Work | Verification |
| --- | --- | --- | --- |
| 2026-10-12 #1 | Planning | Plan drafted. Survey of Capture King (`captureking.zip`, 1.8 MB, downloaded on the owner's instruction to a scratch folder outside the repository; its compiled `.exe` was not run): the project loads and its splash form renders; the run stops at `SocketChat.RemoteHost` (error 438) because `MSWinsock` is not implemented; the project also names `comdlg32.ocx` (`CommonDialog`) | Manual run, headless and `--gui` |
| 2026-10-12 #2 | Construction | Gaps found by Capture King and fixed: native `CommonDialog` (`REQ-0304`) and `Winsock` (`REQ-0305`) controls with Win32 dialogs and loopback sockets; implicit declaration of `For`/`For Each` variables and default-property assignment, `LoadPicture()` (`REQ-0306`). Capture King now loads and shows its splash, game board with pieces, and welcome forms; remaining gaps: black welcome form, button overlay pictures, captions | CTest 368/368 |
| 2026-10-12 #3 | Decision | Owner decisions: WFC is licensed `GPL-3.0-or-later` (`LICENSE`, README); corpus sources live in the new public repository `Thewafflication/vb6games`, added to WFC as the submodule `tests/vb6games`; Capture King added there unmodified with `PROVENANCE.md` (executable and junk files omitted) | Not applicable |
| 2026-10-12 #4 | Construction | `--exit-after <ms>` and `--ui-script <file>` options and a host wake-up timer (sockets poll while a real loop sleeps); `End` in an Unload handler at exit ends quietly; `TC-MP0008-vb6games-captureking` runs Capture King in the Win32 host and requires the splash and game forms to be shown | CTest 369/369 |
| 2026-10-12 #5 | Defect | Windows system colours use the default palette (index 0 to 24) instead of grey for unknown indexes; screenshots repaint before capture; Capture King screenshot added to the README | CTest 369/369 |

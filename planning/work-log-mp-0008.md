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
| 2026-10-12 #6 | Survey | Rol An Go ARENA (`Travaux DirectX VB6 Code.rar`, 7,115,367 bytes, SHA-256 `02a7d813d5986d9ed03bc6e08a3436b7357b08238d935e41981d3f8b5fa0d37f`, plus its `readme.txt`, downloaded on the owner's instruction to a scratch folder and extracted there with 7-Zip; nothing was run): no license in the readme or sources, the author is not the archive maintainer, it references `DirectX 8 for Visual Basic` (`dx8vb.dll`) and ships `dx8vb.dll`, `msvbvm60.dll` and `EX_Time.dll`. Excluded from the corpus under the plan's license rule | Inspection |
| 2026-10-12 #7 | Defect | A form's own BackColor now paints its surface; `a.Picture = b` and `a = b` between controls use the default property (picture) of `b`; captures wait for a freshly shown window to paint (Capture King Welcome form drew black from the splash timer) | CTest 369/369 |
| 2026-10-12 #8 | Verification | `TC-MP0008-vb6games-captureking-script` drives Capture King headless (splash, local game, two pawn moves by drag and drop) and requires a clean, quiet run; corpus projects now run from a copy so the submodule stays clean | CTest 370/370 |
| 2026-10-12 #9 | Construction | `wfc --build` writes the project's version information (company, product, description, copyright, version) into the executable's version resource; `TC-MP0008-package-version` | CTest 371/371 |
| 2026-10-12 #10 | Construction | `wfc --build` also embeds the `IconForm` icon as the executable's icon; `TC-MP0008-package-icon` (Capture King's queen differs from the runtime's) | CTest 372/372 |
| 2026-10-12 #11 | Planning | Draft WPM package and first-release plan (`MP-0008-release-plan.md`) modelled on WCRT's release process; nothing tagged, signed or published | Not applicable |
| 2026-10-12 #12 | Construction | Gaps from Capture King's menus: `Declare`d DLL routines are now called in their DLLs (`REQ-0307`; `ClientToScreen`, `SetCursorPos` ...); a bare `End` inside an `If` block; opening a file for Input, Binary or Random again under another number (only Output and Append conflict, per the VB `Open` documentation); UDP sockets send to broadcast addresses and bind `LocalPort` on first send; corpus 199 and 200 | CTest 374/374 |

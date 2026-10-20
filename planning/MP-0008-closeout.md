# MP-0008 — Project Compatibility: Closeout Record

**Content type:** Milestone closeout record (per `wsp/processes/milestone-closeout-template.md`)

**Milestone or work package:** MP-0008 — Project compatibility (`0.8.0`)

**Status:** Complete (with documented deferrals)

**Completion date:** 2026-10-20

**Inherited baseline:** MP-0007 closeout (`MP-0007-closeout.md`)

**Tested baseline:** windows-x64-debug, 408 of 408 CTest tests passing; the
GitHub Actions matrix (Debug and Release on x86, x64 and ARM64) is green on
the closing commit

**Owner:** Project maintainer

**Approval:** Owner instruction in session, 2026-10-20: complete and close
MP-0008, then tag and push the release

## Outcome

Real VB6 projects load and run. Seven archived games from the public
`vb6games` repository (the submodule `tests/vb6games`) start in native
windows, are played by scripted sessions and are packaged as standalone
programs for x86, x64 and ARM64; the first WPM packages and the first release
(`0.8.0`) follow from this gate.

- **Corpus.** Capture King (GPL), Clix (MIT), Connect 4 (GPL-3.0), Executioner
  (GPL-3.0), Pipes (GPL-3.0), Resta Um (MIT) and SuMBOku (GPL-3.0). Each has a
  `PROVENANCE.md` with source, retrieval, license and omissions. Rol An Go
  ARENA is excluded (no license, needs DirectX 8).
- **Acceptance.** Every game has a screenshot test in real windows
  (`TC-MP0008-vb6games-*`) and a packaged-program test (`TC-MP0008-package-*`).
  Scripted play sessions with expected output cover Resta Um, Connect 4,
  SuMBOku, Executioner and Pipes. Capture King plays whole games of random
  moves and chat to a check mate, and two copies play each other over real
  sockets on a virtual LAN of loopback addresses. The README shows recorded
  play as GIFs made by `scripts/New-PlayGif.ps1` from the same sessions.
- **Native controls and dialogs.** `MSWinsock` and `CommonDialog` are native
  (`REQ-0304`, `REQ-0305`); the Win_Dialogs classes the newer games name
  (`ColorDialog`, `FontDialog`, `OpenFileDialog`, `SaveFileDialog`) are built
  in over the native common dialogs.
- **Project format.** `Declare`d DLL calls, resource files, icons, version
  information, `Object=` and `Reference=` lines; `UserDocument=` and the
  designer items are refused with a message.
- **Migration diagnostics.** DDE members and `LinkMode` raise error 282; the
  Data control's `Refresh`, `UpdateControls` and `UpdateRecord` raise 3170; a
  project with `UserDocument=` is refused (`requirements/language/diagnostics.md`).
- **Matrix.** The corpus passes in Debug on x86, x64 and ARM64, and the whole
  suite also runs in Release on all three (`Release <arch>` jobs).
- **Release machinery.** `scripts/Build-WpmPackage.ps1` builds `wfc` and
  `wfc-games`; the tag-only CI `package` job signs them with the repository
  secret `WPM_RELEASE_PRIVATE_KEY`, verifies them against
  `release_keys/wfc-release.public` and uploads them.
- **Defects found by the corpus and fixed** include ScaleMode geometry,
  control paint and z-order, picture-box client area, `Exit Sub` inside `For`,
  default members of controls and classes, `Input #` field rules, subscripts
  of unallocated arrays in unreached code, and windows of a process started
  with a hidden-window request. They are listed in the work log.

## Scope Accounting

| Planned item | Final disposition | Evidence or later allocation |
| --- | --- | --- |
| Corpus projects with provenance | Complete (seven; Rol An Go ARENA excluded) | `tests/vb6games/*/PROVENANCE.md` |
| `MSWinsock`, `CommonDialog` | Complete | `REQ-0304`, `REQ-0305`, corpus tests |
| Acceptance tests and screenshots of each project | Complete | `TC-MP0008-vb6games-*`, `evidence/screenshots` |
| Project format: resources, icons, version information, references | Complete | `TC-MP0007-package-*`, `TC-MP0008-package-*` |
| Matrix on x86, x64, ARM64 | Complete | CI |
| Migration diagnostics | Complete for DDE, Data control, `UserDocument`; OLE1, property pages, help display and `___MSJetSQLHelp` stay reserved | `TC-MP0002-corpus-209-unsupported-legacy`, `TC-MP0008-userdocument-unsupported` |
| WPM package and release | Complete as machinery; signed on the release tag | `scripts/Build-WpmPackage.ps1`, CI `package` job |
| Native machine code | Deferred | MP-0009 |

## Exit Criteria

| Criterion | Required evidence | Gate | Status |
| --- | --- | --- | --- |
| Each corpus project loads and reaches its acceptance state | Acceptance tests | Required | Pass |
| Every corpus project has provenance and a license record | `PROVENANCE.md` | Required | Pass |
| The corpus passes on x86, x64 and ARM64 | CI | Required | Pass |
| Every variance is traced to an approved scope or defect record | Variances below, work log | Required | Pass |
| The WPM package verifies | CI `package` job: `wpm verify` of the signed packages | Required | Pass at the release tag (see the release record) |
| Behavioral probes retained | None | Roadmap gate | Deferred |

## Variances

| Variance from VB6 or from the source | Reason | Disposition |
| --- | --- | --- |
| `Win_Dialogs` is not cloned; its dialog classes are built into WFC | The library is a separate project whose license was not checked | Accepted; owner asked for built-ins over the native dialogs |
| `captureking/CaptureKing.frm` has changed layout properties | Laid out as the author's user guide shows | Recorded in its `PROVENANCE.md`; the only modified corpus file |
| `pipes.vbp` was written for WFC | The source has no project file | Recorded in its `PROVENANCE.md` |
| Network games run on loopback or a virtual LAN (`WFC_LAN_ADDRESS`, `WFC_LAN_PEERS`) in tests | One machine plays both sides | Test aid only |
| `WFC_RANDOM_SEED` makes `Randomize` repeatable | Scripted play needs the same game each run | Test aid only; unset in normal runs |
| Debug builds are slow (Clix needs about 90 s to start); Release is about ten times faster | Text-reparsing interpreter | Native compilation, MP-0009 |
| Programs that use DDE, the Data control or `UserDocument` stop with a message | Deferred beyond 1.0 by decision of 2026-10-09 | Documented; migration diagnostics |
| SuMBOku's first-cell text has no real-VB6 reference | The real compiler cannot build it here (unlicensed OCX) | Accepted; reference-machine probes |
| Real common dialogs through the built-in classes were exercised with scripted answers only | No unattended way to drive them | Accepted; manual check |
| The ARM64 and x86 corpus waits are causal (`waitfor`), not timed | Slower runners | Test design |

## Deferred Objectives and Accepted Risk

| Item | Impact | Owner | Target milestone or release | Compensating control | Approval |
| --- | --- | --- | --- | --- | --- |
| Native machine code for x86, AMD64 and ARM64 | Programs are interpreted inside the package | Project maintainer | MP-0009 | Packaged runtime | Owner, 2026-10-12 |
| Behavioral probes against a VB6 reference machine | Behavior rests on source reading and documented behavior | Project maintainer | Reference machine before 1.0 | Acceptance tests | Pending |
| `MSCOMCTL` binary persistence, ImageList `Draw` overlay, ListView hover selection, printer dialog, `AddressOf` callbacks, floating-point `Declare` arguments | Some programs differ | Project maintainer | MP-0009 and MP-0010 | Tests of the supported subset | Pending |
| More corpus games (Game of Life, Path Finder, CollideBall, Pairs and Triples) | Wider coverage | Project maintainer | MP-0010 | None needed | Each download needs the owner's approval |
| Windows Authenticode signing and the protected `release` environment | Packages are WPM-signed only | Project maintainer | MP-0010 | WPM signature | Pending |

## Closeout Decision

- **Decision:** Complete with documented deferrals
- **Approver:** Project maintainer
- **Rationale:** Every required exit criterion passes; the deferrals are
  recorded with owners and targets.
- **Next review or release action:** release `0.8.0` (tag, signed packages);
  then `MP-0009` (native compilation).

## References

- [MP-0008 plan](MP-0008-project-compatibility.md)
- [Work log](work-log-mp-0008.md)
- [WPM package and first release plan](MP-0008-release-plan.md)

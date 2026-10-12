# MP-0008 — Project Compatibility

**Content type:** Milestone work plan  
**Status:** Planned  
**Target baseline:** 0.8.0  
**Planned period:** 2026-10-12 through milestone closeout  
**Inherited baseline:** MP-0007 closeout (`MP-0007-closeout.md`), `master` at the closeout commit  
**Owner:** Project maintainer  
**Approval:** Plan drafted on the maintainer's instruction, 2026-10-12; formal approval pending

## Objective and Scope

Real VB6 projects load and run on WFC without source changes. The first corpus
is VB6 games archived on SourceForge, chosen because they exercise forms,
controls, graphics, timers, input and sometimes networking, and make a good
demonstration. After the exit gate the packaged games are published as a WPM
package with the first release.

Native compilation is not part of this milestone (see MP-0009). Projects run on
the interpreter, directly or as packaged programs (`REQ-0303`).

### Inherited (do not redo)

Forms, intrinsic controls, ActiveX hosting, `MSComctlLib`, the COM server,
packaged programs and the CI matrix (x86, x64, ARM64) from MP-0002 to MP-0007.

### Corpus

| Project | Source | License | Size | Known needs |
| --- | --- | --- | --- | --- |
| Capture King (network chess) | [VB6 Game Archives](https://sourceforge.net/projects/vb6gamearchive/files/Capture%20King/) | GPL, credit to Christopher McKirgan (per its readme) | 1.8 MB | `MSWinsock`, `CommonDialog`, images, `.frx` pictures, 5,000 lines of forms |
| Rol An Go ARENA (tactical RPG) | VB6 Game Archives, `Tactical Role Playing` | **None stated**; the archive maintainer says they did not write it and the source files carry no license | 7.1 MB | **Excluded** (2026-10-12): unclear license, and it needs the DirectX 8 for Visual Basic library (`dx8vb.dll`) and bundles Microsoft binaries |
| Further archived games | `vb6archives`, `VB 6 App Archive`, other SourceForge projects | Per project | — | Selected after the first two run |

Each corpus project keeps its source unmodified in the `vb6games` repository
([Thewafflication/vb6games](https://github.com/Thewafflication/vb6games)),
which WFC includes as the submodule `tests/vb6games`. A project directory holds
`PROVENANCE.md` (URL, retrieval date, SHA-256 of the archive, license and
attribution, omitted files); WFC holds its acceptance test (scripted run,
screenshot, or both). Compiled binaries in an archive are never kept or run.
WFC itself is `GPL-3.0-or-later` (owner decision, 2026-10-12), which is
compatible with the GPL games; each project keeps its author's license.

### Requirement set

| Group | Requirements | Disposition |
| --- | --- | --- |
| Gaps exposed by the corpus | Recorded as ordinary requirements with tests as found (`REQ-0304` onward) | In scope |
| `MSWinsock` (`mswinsck.ocx`) and `CommonDialog` (`comdlg32.ocx`) | New control requirements | In scope: needed by the first project |
| Project format closure | References, resources, manifests, icons, version information | In scope |
| Migration diagnostics (`WFC9007` and the removed-feature list) | `diagnostics.md` | In scope |
| Carried deferrals | Accessibility, per-monitor DPI, drawing fidelity, MDI menu merging, MSCOMCTL binary persistence, OLE drag and drop, printer graphics | Triaged by what the corpus needs |

### Plan decisions

- **Corpus drives scope.** A gap is fixed when a corpus project needs it;
  speculative work is not scheduled.
- **Native controls for the common OCXs.** `MSWinsock` and `CommonDialog` are
  implemented natively behind the extender like `MSComctlLib` (ADR-0006), so
  projects need no 32-bit OCX on any architecture. `Object=` lines for them
  resolve to the native classes.
- **Headless first.** Behavior is tested on the headless host (the sockets use
  loopback); Win32 hosts show dialogs and windows.
- **Acceptance tests.** A project passes when a scripted run reaches a defined
  state (forms load, events fire, expected output) and a screenshot is retained.

## Baseline and Assumptions

- Sources are fetched by the maintainer's instruction from the named archive
  pages; nothing from an archive is executed.
- No VB6 machine exists, so expected behavior comes from reading the source and
  the documented control behavior.

## Deliverables

| Deliverable | Completion evidence |
| --- | --- |
| Corpus projects with provenance | `tests/projects/*`, `PROVENANCE.md` per project |
| Controls and language gaps the corpus exposes | Requirement records, corpus tests |
| `MSWinsock` and `CommonDialog` | Requirement records, tests |
| Acceptance tests and screenshots of each project | CTest cases, `evidence/screenshots` |
| Migration diagnostics | Catalog entries, tests |
| WPM package of the packaged projects and the release | Package verification record, release notes |
| Closeout record | `MP-0008-closeout.md` |

## Work Breakdown and Sequence

| Stage | Outcome | Completion condition |
| --- | --- | --- |
| 1. Survey | Each candidate project loaded; gaps listed in the work log | Gap list per project |
| 2. First project | Capture King runs to its acceptance state | Test and screenshot |
| 3. More projects | Rol An Go ARENA and further games, one at a time | Test and screenshot each |
| 4. Project format | Resources, icons, version information, references, diagnostics | Tests |
| 5. Matrix | The corpus on x86, x64, ARM64 CI | Green matrix |
| 6. Package and release | WPM package verified; release prepared | Owner approval of the release |
| 7. Closeout | Deferrals recorded | Required gates pass |

## Requirement and Verification Allocation

| Requirement or objective | Design or implementation allocation | Verification | Required gate |
| --- | --- | --- | --- |
| Corpus projects run | Interpreter, forms, controls | Acceptance tests | Yes |
| `MSWinsock`, `CommonDialog` | `src/interpreter` native controls | Corpus tests | Yes |
| Provenance and license records | `PROVENANCE.md` | Inspection | Yes |
| Package | WPM manifest | `wpm verify` | Yes |

## Roles and Review

| Role | Assignment | Responsibility or independence condition |
| --- | --- | --- |
| Owner | Project maintainer | Scope, corpus selection, licensing decision, release approval |
| Reviewer | Documented approved exception (single-maintainer project) | Review requirement records |
| Verifier | Project maintainer | Execute and retain build/test evidence |

## Risks and Controls

| Risk | Impact | Planned control | Owner or trigger |
| --- | --- | --- | --- |
| Licenses of archived projects | Obligations on redistribution | WFC is GPL-3.0-or-later; each project keeps its author's license and credit in the `vb6games` repository and in the release package; a project whose license is unclear or incompatible is excluded | Stage 2 |
| Archive code of unknown quality or origin | Surprising behavior | Never run archive binaries; read each project before running it | Stage 1 |
| Games need DirectX, `winmm` or sound | Projects do not run | Survey first; implement the subset a project needs or exclude the project with a recorded reason | Stage 1 |
| Network games need a peer | No end-to-end test | Test against loopback with a second WFC process | Stage 2 |
| Size of the gap list | Delay | One project at a time; replan after the second | Stage 3 |

## Estimate and Forecast

Forecast 30–50 increments, dominated by gaps found in the first two projects.
Replan after Rol An Go ARENA.

## Execution and Evidence

- Local x64 debug configure, build and CTest per `AGENTS.md`; x86 and ARM64 are
  validated by GitHub Actions.
- Progress is recorded in [work-log-mp-0008.md](work-log-mp-0008.md).

## Rollback and Recovery

Corpus projects are separate directories and tests; revert a defective
increment as a normal Git change.

## Exit Criteria

| Criterion | Required evidence | Gate | Status |
| --- | --- | --- | --- |
| Each corpus project loads and reaches its acceptance state | Acceptance tests | Required | Not started |
| Every corpus project has provenance and a license record | `PROVENANCE.md` | Required | Not started |
| The corpus passes on x86, x64 and ARM64 | CI | Required | Not started |
| Every variance is traced to an approved scope or defect record | Work log, closeout | Required | Not started |
| The WPM package verifies | Package verification | Required | Not started |
| Behavioral probes retained | None | Roadmap gate | Deferred |

## Deferred Objectives

| Objective | Impact | Owner | Target milestone or release | Compensating control | Approval |
| --- | --- | --- | --- | --- | --- |
| Native compilation | Interpreted projects | Project maintainer | MP-0009 | Packaged programs | Owner, 2026-10-12 |
| VB6 reference probes | Behavior unconfirmed | Project maintainer | Reference machine before 1.0 | Acceptance tests | Pending |

## Change Control

Replan when a corpus project needs a feature outside the profile, a license
blocks a project, or a stage runs more than twice its forecast.

## References

- [Roadmap to 1.0.0](roadmap-1.0.md)
- [MP-0007 closeout record](MP-0007-closeout.md)
- [ADR-0006](../architecture/adr-0006-activex-hosting.md)
- [VB6 Game Archives](https://sourceforge.net/projects/vb6gamearchive/)

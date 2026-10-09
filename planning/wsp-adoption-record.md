# WSP Adoption Record

**Content type:** Project adoption record

**Project:** WFC — Waughtal Foundation Classes

**WSP release:** 1.4.0

**WSP baseline:** Immutable commit `f009399dd1406219571ac978bfee85e85bbdfeac`

**Submodule path:** `wsp/`

**Pinned commit:** `f009399dd1406219571ac978bfee85e85bbdfeac`

**Status:** Adopted for MP-0001; upgraded to 1.4.0 on 2026-10-09 with the
migration gaps recorded under [WSP 1.4.0 upgrade](#wsp-140-upgrade)

**Approval:** Initial 0.1.0 foundation change

## Common Baseline

| Requirement set or practice | Applicability | Project artifact or scope |
| --- | --- | --- |
| Common requirements management | Yes | `requirements/` and the 1.0 compatibility profile |
| WSP software lifecycle | Yes | Versioned milestones, controlled changes, closeout, releases, and support |
| Project process | Yes | Planning through maintenance and improvement |
| Documentation requirements | Yes | Project-controlled technical, user, compatibility, and release information |
| Documentation style and identifiers | Yes | Markdown authority and stable project identifiers |
| Testing requirements | Yes | Reference probes, unit/integration tests, CTest, CI, and retained evidence |

## Selected Profiles

| Profile | Selected | Project scope or rationale |
| --- | --- | --- |
| Personal process | Yes | Requirement-sized discovery and implementation work under milestone plans |
| Security/DFS | Yes | Untrusted VB source/projects, COM activation, runtime loading, dependencies, and release integrity |
| C source style | No | WFC-owned production source is C++; dependency and generated C remain outside this profile |
| PowerShell style | Yes | Project-owned build, discovery, test, evidence, packaging, and reporting scripts |
| CMake style | Yes | Top-level native build and target definitions |
| Windows version resources | Yes | Shipped WFC executables and DLLs |
| Windows code signing and Defender | Yes | Public Windows binaries and packages; implementation is deferred until release work |
| Common tools | Yes | Traceability, source-quality, evidence, checksum, and documentation tools where applicable |

## Requirement-Set Dispositions

This initial record adopts every common requirement. Individual controls that
cannot yet be satisfied are explicitly deferred to the milestone that first
needs their evidence; they are not reported as passing in MP-0001.

| WSP requirement set | Disposition | Project artifact or completion condition |
| --- | --- | --- |
| `WSP-REQM-0001`–`WSP-REQM-0010` | Applicable | Requirement records, explicit tailoring, traceability validation, and release baselines |
| `WSP-PROC-0001`–`WSP-PROC-0006` | Applicable | Milestone plans, Git history, review, and defect records |
| `WSP-PROC-0007`–`WSP-PROC-0010` | Deferred | Release readiness, support, and retrospective records by MP-0009/MP-0010 |
| `WSP-PSP-0001`–`WSP-PSP-0009` | Applicable | Milestone work plans, reviews, work records where useful, and closeouts |
| `WSP-DOC-0001`–`WSP-DOC-0012` | Deferred | Reproducible release-document pipeline by MP-0009 |
| `WSP-DOC-0013` | Not applicable | No PAdES-signed PDF requirement has been selected |
| `WSP-TEST-0001`–`WSP-TEST-0005` | Applicable | Requirement-linked specifications and repeatable local execution |
| `WSP-TEST-0006`–`WSP-TEST-0018` | Deferred | CI, reporting, retention, target matrix, and debug evidence introduced through MP-0008 |
| `WSP-SEC-0001`–`WSP-SEC-0014` | Deferred | Controlled DFS and derived security evidence before 0.1.0 closes; release-response controls by MP-0009 |
| `WSP-WINRES-0001`–`WSP-WINRES-0012` | Deferred | Generated and verified resources before the first distributed binary baseline |
| `WSP-SIGN-0001`–`WSP-SIGN-0018` | Deferred | Signing, scanning, and trust dispositions before public release artifacts |
| `WSP-TOOL-0001`–`WSP-TOOL-0009` | Deferred | Select and verify each common-tool invocation as build/test automation enters scope |
| `WSP-PROC-0011` | Applicable | Upstream issues for workarounds in Thewafflication-owned dependencies (`wsp`, `wcrt`) |
| `WSP-STYLE-0001`–`WSP-STYLE-0007` | Applicable — migration gap | See [WSP 1.4.0 upgrade](#wsp-140-upgrade) |
| `WSP-LANG-0002` (C++) | Applicable — migration gap | `src/`, `tests/*.cpp`; C++20 |
| `WSP-LANG-0004` (YAML) | Applicable — migration gap | `.github/workflows/`, `.pre-commit-config.yaml` |
| `WSP-LANG-0005` (JSON) | Applicable | `CMakePresets.json`; generated `evidence/reference/*.json` excluded as tool output |
| `WSP-LANG-0007` (PowerShell) | Applicable; no owned scripts yet | The retired MP-0001 discovery scripts in `tools/` are excluded (see [Owned-source exclusions](#owned-source-exclusions)); PowerShell added for the commit gate is in scope |
| `WSP-LANG-0009` (Visual Basic) | Not applicable to owned source | `.bas`/`.cls`/`.vbp` files under `tests/` are compiler test fixtures (deliberately varied VB6 input), excluded as owned fixtures |
| `WSP-LANG-0010` (CMake) | Applicable — migration gap | `CMakeLists.txt`, `cmake/`, `tests/**/*.cmake` |
| `WSP-LANG-0001`, `0003`, `0006`, `0008`, `0011` | Not applicable | WFC owns no C, Python, Make, C#, or other-language source |
| `WSP-CHECK-0001`–`WSP-CHECK-0008` | Applicable — migration gap | See [WSP 1.4.0 upgrade](#wsp-140-upgrade) |
| `WSP-SAST-0001`–`WSP-SAST-0006` | Applicable — migration gap | clang-tidy over `src/` and `tests/` |
| `WSP-SEC-0015`–`WSP-SEC-0016` | Applicable — migration gap | MSVC hardening for `wfc.exe` and verification with `Test-PeHardening.ps1` |
| `WSP-TEST-0019`–`WSP-TEST-0021` | Applicable | `wfc` reads VB `Input`/`Line Input` from standard input; covered when console input enters scope |
| `WSP-INFO-*` (information for users) | Selection pending maintainer decision | WFC will ship a CLI, compiler diagnostics, and compatibility documentation |
| `WSP-UX-*` (UX/UI) | Selection pending maintainer decision | WFC's CLI and diagnostics, and the forms runtime from MP-0004 |

## Tailoring Decisions

### WSP C source style profile

- **Disposition:** Not applicable
- **Rationale:** WFC is a C++ project. WCRT and other submodules own their C
  source and apply their own controlled style baselines.
- **Impact:** WSP's C-specific Doxygen and physical-line checks do not govern
  WFC C++ files.
- **Compensating control:** WFC shall define and automate an equivalent C++
  warnings, formatting, documentation, and analysis policy during MP-0001.
- **Owner:** Project maintainer
- **Target release or completion condition:** N/A
- **Approval:** Initial 0.1.0 foundation change

### Deferred requirements

- **Disposition:** Deferred
- **Rationale:** Initial adoption records the project at the start of MP-0001;
  many release and continuous-verification controls do not yet have an
  implementation or evidence path.
- **Impact:** Deferred controls cannot support a completion or compatibility
  claim.
- **Compensating control:** MP-0001 exit criteria require the initial build,
  architecture, dependency, test, and evidence paths. Each milestone closes,
  revises, or carries every applicable deferral without calling it verified.
- **Owner:** Project maintainer
- **Target release or completion condition:** As allocated above and in the
  roadmap to 1.0.0
- **Approval:** Initial 0.1.0 foundation change

## WSP 1.4.0 upgrade

WSP 1.2.0–1.4.0 add requirements that WFC did not meet when the pin moved.
WSP 1.4.0 permits adoption with existing violations provided they are recorded
as migration gaps rather than declared compliant. Measured on 2026-10-09:

| Gap | Requirement | Measured state | Planned closure |
| --- | --- | --- | --- |
| Overlength lines | `WSP-STYLE-0001` | 2,493 lines over 80 characters (`src/evaluator.cpp` 1,980, `tests/evaluator_tests.cpp` 405, `CMakeLists.txt` 57, others 51); `Test-SourceStyle.ps1` reports no other `WSP-STYLE` findings | **Closed 2026-10-09** (`c62755f`): `Test-SourceStyle.ps1` passes for `src`, `include`, `tests`, `cmake`, `.github`, and the root build/configuration files. Not yet enforced by a lint stage (see below) |
| Formatter configuration | `WSP-STYLE-0004`, `WSP-LANG-0002`, `WSP-LANG-0010` | No `.clang-format`, `.editorconfig`, or gersemi configuration | **Closed 2026-10-09** (`c62755f`): `.clang-format` (clang-format 22.1.3), `.gersemirc` (gersemi 0.29.2), `.editorconfig`, `.gitattributes`; check-mode runs join the lint stage |
| Structured documentation | `WSP-STYLE-0006`, `WSP-LANG-0002` | No `Doxyfile`; most functions lack Doxygen contracts | Add a strict `Doxyfile` and document owned C++ incrementally, file by file |
| Commit gate | `WSP-CHECK-0001`–`0007` | No `.pre-commit-config.yaml`, no canonical lint/build/test scripts, no check inventory | Add project-owned scripts shared by hooks and CI, plus a check inventory |
| CI lint stage | `WSP-STYLE-0007`, `WSP-CHECK-0008` | CI builds and tests on x86/x64/ARM64 but runs no lint | Add a lint job calling the canonical lint script |
| Static analysis | `WSP-SAST-0001`–`0006` | clang-tidy not configured | Ninja-based analysis preset using the WSP clang-tidy baseline |
| Build hardening | `WSP-SEC-0015`–`0016` | MSVC defaults (`/GS`, ASLR, NX) are on; Control Flow Guard and binary verification are not | Enable `/guard:cf` and verify with `Test-PeHardening.ps1` in CI |

Until each gap closes, the affected requirement is not reported as passing.

### Owned-source exclusions

| Path | Reason | Approval |
| --- | --- | --- |
| `wsp/`, `wcrt/` | Pinned submodules owned by their own projects | Ownership boundary |
| `out/` | Build output | Ownership boundary |
| `evidence/reference/*.json` | Tool-generated reference evidence | Ownership boundary (generated) |
| `tests/corpus/`, `tests/fixtures/` (`.bas`, `.cls`, `.vbp`, `.expected`) | VB6 compiler test fixtures: deliberately varied input and exact expected output | Owned fixture boundary |
| `tools/*.ps1` | Retired MP-0001 discovery scripts kept for provenance; must not be re-run (see `tools/README.md`) | Maintainer decision, 2026-10-09 |

## Baseline History

| Date | WSP baseline | Project change | Summary |
| --- | --- | --- | --- |
| 2026-08-27 | `1.1.0` / `8c2adb4afb9f95a5632ec783e37a79c29b1f90f5` | Initial 0.1.0 foundation change | Adopt the latest controlled WSP release and replace the earlier post-release checkout with the immutable tag. |
| 2026-10-09 | `1.4.0` / `f009399dd1406219571ac978bfee85e85bbdfeac` | WSP upgrade (maintainer request) | Adopt WSP 1.4.0; disposition the 140 requirements added in 1.2.0–1.4.0 and record the migration gaps above. |

The current baseline, pinned commit, and `wsp` gitlink shall agree. Upgrades
require a change-impact review covering new or changed requirements, profiles,
tools, templates, and project dispositions.

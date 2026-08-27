# WSP Adoption Record

**Content type:** Project adoption record

**Project:** WFC — Waughtal Foundation Classes

**WSP release:** 1.1.0

**WSP baseline:** Immutable commit `8c2adb4afb9f95a5632ec783e37a79c29b1f90f5`

**Submodule path:** `wsp/`

**Pinned commit:** `8c2adb4afb9f95a5632ec783e37a79c29b1f90f5`

**Status:** Adopted for MP-0001

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

## Baseline History

| Date | WSP baseline | Project change | Summary |
| --- | --- | --- | --- |
| 2026-08-27 | `1.1.0` / `8c2adb4afb9f95a5632ec783e37a79c29b1f90f5` | Initial 0.1.0 foundation change | Adopt the latest controlled WSP release and replace the earlier post-release checkout with the immutable tag. |

The current baseline, pinned commit, and `wsp` gitlink shall agree. Upgrades
require a change-impact review covering new or changed requirements, profiles,
tools, templates, and project dispositions.

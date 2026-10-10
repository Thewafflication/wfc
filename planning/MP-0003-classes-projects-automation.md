# MP-0003 — Classes, Projects, and Automation

**Content type:** Milestone work plan  
**Status:** In progress  
**Target baseline:** 0.3.0  
**Planned period:** 2026-10-10 through milestone closeout  
**Inherited baseline:** MP-0002 closeout (`MP-0002-closeout.md`), `master` at `4a8784e` plus the closeout commit  
**Owner:** Project maintainer  
**Approval:** Plan revised from the roadmap on the maintainer's instruction, 2026-10-10; formal approval pending

## Objective and Scope

Multi-file projects with class modules, objects, properties, events,
interfaces, collections, and Automation execute end to end, and the `App`,
`Global`, `Screen`, and `Clipboard` service objects behave as their type-library
contracts require.

### What MP-0002 already delivered (do not redo)

`.vbp` loading with `.bas` and `.cls` members and per-module namespaces;
fields, methods, `Property Get/Let/Set` (indexed), `New`, `Me`,
`Class_Initialize`/`Class_Terminate`, `Public`/`Private`; `Implements`;
`Event`/`RaiseEvent`/`WithEvents`; predeclared (`VB_PredeclaredId`) instances;
`As New` auto-instantiation; `App` metadata from the `.vbp`; and emulated
`Scripting.Dictionary`, `Scripting.FileSystemObject`, `VBScript.RegExp`, and
`Collection` (`REQ-0283`).

### Plan corrections to the roadmap

- **Class inheritance is removed from scope.** VB6 has interface
  implementation (`Implements`) but no implementation inheritance, so there is
  nothing to be compatible with. `Implements` conformance is in scope.
- **Automation is split into a client and a server.** Late-bound client
  access to real COM objects (`CreateObject`/`GetObject` through `IDispatch`)
  is in scope for this milestone. Exposing WFC classes as COM servers
  (registration, class factories, out-of-process hosting) requires an ADR and
  is included only as far as that ADR decides.
- **Language limits from MP-0002** (per-module `Option Explicit`,
  `VB_GlobalNameSpace`, string `#Const`) are in scope, because they affect
  multi-module projects.

### Included Work

1. **Class-language completeness.** Default members (`VB_UserMemId = 0`),
   enumerators for `For Each` over class instances (`VB_UserMemId = -4`,
   `NewEnum`), `Friend` members, `Property Let/Set` rules, multiple
   `Implements`, interface conformance errors, `Is`/`TypeOf` over interfaces,
   `Class_Terminate` ordering, `CallByName`, per-module `Option Explicit`,
   `VB_GlobalNameSpace` members, and string-valued `#Const`.
2. **Service objects.** The `App` members of `REQ-0058`, `Global` of
   `REQ-0067` (`Forms`, `Load`/`Unload`, `LoadResString`, `LoadResData`,
   headless `LoadPicture`/`SavePicture`), `Clipboard` text of `REQ-0052`, and
   the non-visual `Screen` members of `REQ-0051`.
3. **Automation client.** `CreateObject`, `GetObject`, late-bound property
   get/put/call through `IDispatch::Invoke` with Variant marshalling (scalars,
   strings, dates, arrays, objects, `ByRef`), COM errors mapped to VB run-time
   errors, and object lifetime via reference counting.
4. **COM identity and type information.** `IUnknown`/`IDispatch` behavior
   visible to VB code (`Is`, `TypeName`, `Nothing`) and `Reference=` lines in
   the `.vbp` for early binding where a type library can be read.
5. **Automation server decision.** ADR on exposing WFC classes (in-process
   registration versus out-of-process), with a thin proof only if accepted.

### Excluded or Deferred Work

- Forms, controls, and any UI surface (MP-0004), including `MsgBox`/`InputBox`
  and `SendKeys`.
- ActiveX component authoring, `stdole` font/picture contracts, and OLE
  hosting (MP-0005).
- Event sinks on real COM objects (connection points) beyond what the ADR
  accepts; recorded as a risk below.
- Native compilation and VBRUN ABI (MP-0007).

## Baseline and Assumptions

- The interpreter remains the execution vehicle; no AST rewrite is planned.
- COM access is Windows-only and uses the platform COM runtime. Other hosts
  report error 429 for real ProgIDs while keeping the emulated classes.
- Emulated classes (`Scripting.*`, `VBScript.RegExp`) stay as built-in sources
  and take precedence over registered COM servers of the same ProgID, so the
  corpus remains deterministic.

## Deliverables

| Deliverable | Owner | Completion evidence |
| --- | --- | --- |
| Class-language increments with corpus cases | Project maintainer | New corpus programs pass in CTest |
| `App`/`Global`/`Clipboard`/`Screen` members | Project maintainer | Member-inventory audit against `REQ-0051`/`0052`/`0058`/`0067` |
| COM late-binding client | Project maintainer | CTest cases using system-provided COM objects |
| ADR: Automation server | Project maintainer | Reviewed ADR |
| Requirement records for each increment | Project maintainer | `requirements/language/` entries and diagnostics catalog |

## Work Breakdown and Sequence

| Stage | Outcome | Completion condition |
| --- | --- | --- |
| 1. Class language | Default members, enumerators, `Friend`, conformance errors, per-module `Option Explicit`, `VB_GlobalNameSpace`, string `#Const` | Corpus cases and unit tests pass |
| 2. Service objects | Contracted `App`, `Global`, `Clipboard`, `Screen` members exist | Inventory audit has no missing non-visual member |
| 3. COM client | Real ProgIDs can be created and driven late-bound | CTest cases against stock Windows COM objects pass |
| 4. Identity and typelib | `Reference=` lines resolve; identity operators behave | Corpus cases pass |
| 5. Server ADR | Decision recorded | ADR reviewed |
| 6. Closeout | Evidence and successor input retained | Closeout record approved |

Stages 1 and 2 may interleave; stage 3 follows stage 2 because it reuses
`Global`'s `CreateObject` plumbing.

## Requirement and Verification Allocation

| Requirement or objective | Design or implementation allocation | Verification | Required gate |
| --- | --- | --- | --- |
| `REQ-0200`, `REQ-0203`–`REQ-0206`, `REQ-0283` (inherited) | Interpreter class support | Existing corpus keeps passing | Yes |
| New class-language requirements (`REQ-0284` onward) | `src/interpreter/` | Corpus and unit tests | Yes |
| `REQ-0051`, `REQ-0052`, `REQ-0058`, `REQ-0067` | Service-object sources in `builtin_class_sources.hpp` and native hooks | Member-inventory audit plus behavior programs | Yes (non-visual members) |
| `REQ-0068` (`VBControlExtender`) | Not applicable until controls exist | Deferred to MP-0004 | No |
| Automation client | New `src/interpreter/com_automation.cpp` (Windows) | CTest against stock COM objects | Yes |

## Roles and Review

| Role | Assignment | Responsibility or independence condition |
| --- | --- | --- |
| Owner | Project maintainer | Scope, decisions, completion evidence |
| Reviewer | Documented approved exception (single-maintainer project) | Review requirement records and ADR |
| Verifier | Project maintainer | Execute and retain build/test evidence |

## Risks and Controls

| Risk | Impact | Planned control | Owner or trigger |
| --- | --- | --- | --- |
| COM tests depend on host-installed servers | Flaky CI | Use only components shipped with Windows and skip with a documented marker when absent | Stage 3 |
| Variant marshalling bugs corrupt memory | Crashes | Wrap `VARIANT` in an RAII type; fuzz the new surface; x86/ARM64 checked in Actions | Stage 3 |
| Interpreter text re-parsing makes class-heavy code slow | Timeouts | Keep tests small; speed work is allocated to MP-0008 | Ongoing |
| Event sinks on COM objects need connection points | Scope growth | Defer behind the ADR; client without events first | Stage 3 |
| Automation server needs registry writes | Security, test isolation | ADR decides; no registry writes in CTest | Stage 5 |

## Estimate and Forecast

MP-0002 landed roughly 140 small verified increments. MP-0003 is forecast at a
similar order of magnitude for stages 1–2 (small, language-level) and a few
dozen increments for stage 3. Replan when stage 3's marshalling surface shows a
missing Variant subtype or when the server ADR changes the architecture.

## Execution and Evidence

- Local x64 debug configure, build, and CTest per `AGENTS.md`; x86 and ARM64
  are validated by GitHub Actions.
- Each increment adds a corpus program and, where applicable, a requirement
  record and diagnostics-catalog entries, and is committed and pushed after
  local verification.
- Increments are logged in [work-log-mp-0003.md](work-log-mp-0003.md).

## Rollback and Recovery

Revert a defective increment as a normal Git change. COM support is isolated in
one translation unit behind a `_WIN32` guard so it can be disabled without
touching the emulated classes.

## Exit Criteria

| Criterion | Required evidence | Gate | Status |
| --- | --- | --- | --- |
| Multi-file projects with classes, interfaces, events, default members, and enumerators run end to end | Corpus | Required | In progress |
| Language limits carried from MP-0002 closed | Corpus and `REQ-0283` update | Required | Not started |
| Non-visual `App`/`Global`/`Clipboard`/`Screen` members present | Inventory audit | Required | In progress |
| Late-bound Automation client works against stock COM objects | CTest | Required | Not started |
| Server decision recorded | ADR | Required | Not started |
| Diagnostics catalog current | CTest catalog check | Required | Ongoing |

## Deferred Objectives

| Objective | Impact | Owner | Target milestone or release | Compensating control | Approval |
| --- | --- | --- | --- | --- | --- |
| `VBControlExtender` (`REQ-0068`), visual `Screen` members | No control hosting | Project maintainer | MP-0004 | None | Pending |
| COM event sinks, if the ADR defers them | Cannot handle events from real COM objects | Project maintainer | MP-0005 | Emulated classes only | Pending |

## Change Control

Replan when a new third-party dependency is introduced, when the server ADR
selects out-of-process hosting, or when a requirement is added or removed.

## References

- [Roadmap to 1.0.0](roadmap-1.0.md)
- [MP-0002 closeout record](MP-0002-closeout.md)
- [REQ-0283 — Multi-module projects](../requirements/language/req-0283-multi-module-projects.md)
- `wsp/processes/milestone-plan-template.md`

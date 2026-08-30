# Work Log — MP-0002 VBA Strings Functions

**Content type:** Work log (per `wsp/processes/work-log-template.md`)

**Milestone or work package:** MP-0002 — Core VB/VBA language execution; `Strings`
module (`REQ-0071`) intrinsic function build-out.

**Period:** 2026-08-29 (active session).

**Starting baseline:** `89aff7f` — "Pre-authorize local cmake/ctest/wfc runs".

**Author:** Claude (Overlord cross-project assistant), on behalf of the owner.

**Status:** Active

This log records the chronological execution of an autonomous session that
extends the evaluator's `Strings` intrinsic surface. It supplements, and does
not replace, the accepted MP-0002 plan, the controlled Git history, and the
retained CTest evidence.

## Work Performed

| Date or order | Phase | Activity | Output |
| --- | --- | --- | --- |
| 2026-08-29 #1 | Construction | Verify in-progress `Space`/`String` slice, add matching CLI test and README examples | Commit `e06f5eb` |
| 2026-08-29 #2 | Process | Pre-authorize local `cmake`/`ctest`/`wfc` runs in AGENTS.md and project settings | Commit `89aff7f` |
| 2026-08-29 #3 | Construction | Add `InStr` (2/3-arg, honors `Option Compare Text`); unit + CLI tests, README | Commit `b3cfdc7` |
| 2026-08-29 #4 | Construction | Add `StrComp` (binary/text compare, returns -1/0/1); unit + CLI tests, README | Commit `bbdfdf2` |
| 2026-08-29 #5 | Construction | Add `Replace` (3-arg form, honors `Option Compare Text`); unit + CLI tests, README | Commit `3b8ed18` |
| 2026-08-29 #6 | Construction | Add `Hex`/`Hex$` and `Oct`/`Oct$` radix conversions (`REQ-0072`); unit + CLI tests, README | Commit `4ed283c` |
| 2026-08-29 #7 | Construction | Add `Str`/`Str$` number-to-string with VB6 leading-space rule (`REQ-0072`); unit + CLI tests, README | Commit `1449c35` |
| 2026-08-29 #8 | Construction | Expose `VbCompareMethod` constants and explicit `InStr`/`StrComp` compare arguments | Commit `451e0f6` |
| 2026-08-29 #9 | Construction | Complete `Replace` start, count, and compare arguments | Commit `fd8fb27` |
| 2026-08-29 #10 | Construction | Add `InStrRev` with bounded reverse search and comparison selection | Commit `beebb9e` |
| 2026-08-29 #11 | Construction | Add explicit `Long`-bounded `Val` conversion subset | Commit `b10e5d2` |
| 2026-08-29 #12 | Construction | Add `Abs` and `Sgn` for the current `Long` model | Commit `d77a89f` |
| 2026-08-29 #13 | Construction | Add `CStr` for every current evaluator value type | Commit `cd59a9d` |
| 2026-08-29 #14 | Construction | Add `CLng` identity, Boolean, and strict decimal String conversion | Commit `205f227` |
| 2026-08-29 #15 | Construction | Add `CBool` identity, `Long`, and strict String conversion | Commit `11bd3f3` |
| 2026-08-29 #16 | Construction | Add bounded `Long`-to-`CByte` conversion | Commit `850fd6d` |
| 2026-08-30 #17 | Construction | Add `CInt` identity, Boolean, and strict String conversion bounded to the Int16 range | Commit `3315931` |
| 2026-08-30 #18 | Process | Pre-authorize `git`/`gh` and general shell use in project settings | Commit `4212993` |
| 2026-08-30 #19 | Construction | Add `IsNumeric` information predicate over the current value model | Commit `ee65241` |
| 2026-08-30 #20 | Construction | Add `TypeName` returning the VB6 type-name string | Commit `ea591de` |
| 2026-08-30 #21 | Construction | Add `VarType` type codes and expose `vbLong`/`vbBoolean`/`vbString` constants | Commit `de6bc93` |
| 2026-08-30 #22 | Process | Record increments 17-21 in the work log with running token estimates | Commit `f85b654` |
| 2026-08-30 #23 | Construction | Add `IIf(condition, truepart, falsepart)` selection with strict Boolean condition | Commit `3ff3fea` |
| 2026-08-30 #24 | Construction | Add `Choose(index, ...)` 1-based selection with explicit out-of-range error | Commit `ef5bea5` |
| 2026-08-30 #25 | Construction | Add `Switch(expr, value, ...)` first-match selection with explicit no-match error | Commit `d954272` |
| 2026-08-30 #26 | Process | Record increments 22-25 in the work log with token estimates and Null-to-error decision | Commit `6045d9c` |
| 2026-08-30 #27 | Construction | Add `Int`/`Fix` truncation over the current `Long` domain | Commit `48ef2b7` |
| 2026-08-30 #28 | Construction | Expose `AscW`/`ChrW` as ASCII-range equivalents of `Asc`/`Chr` | Commit `0a60bae` |

## Verification Log

| Date | Configuration or method | Result | Evidence or failure reference |
| --- | --- | --- | --- |
| 2026-08-29 | `ctest --preset windows-x64-debug` (post `e06f5eb`) | Pass (30/30) | Local CTest run |
| 2026-08-29 | `ctest --preset windows-x64-debug` (post `b3cfdc7` InStr) | Pass (31/31) | Local CTest run |
| 2026-08-29 | `ctest --preset windows-x64-debug` (post `bbdfdf2` StrComp) | Pass (32/32) | Local CTest run |
| 2026-08-29 | `ctest --preset windows-x64-debug` (post `3b8ed18` Replace) | Pass (33/33) | Local CTest run |
| 2026-08-29 | `ctest --preset windows-x64-debug` (post `4ed283c` Hex/Oct) | Pass (34/34) | Local CTest run |
| 2026-08-29 | `ctest --preset windows-x64-debug` (post `1449c35` Str) | Pass (35/35) | Local CTest run |
| 2026-08-29 | `ctest --preset windows-x64-debug` (post `451e0f6` compare methods) | Pass (36/36) | Local CTest run |
| 2026-08-29 | `ctest --preset windows-x64-debug` (post `fd8fb27` Replace options) | Pass (37/37) | Local CTest run |
| 2026-08-29 | `ctest --preset windows-x64-debug` (post `beebb9e` InStrRev) | Pass (38/38) | Local CTest run |
| 2026-08-29 | `ctest --preset windows-x64-debug` (post `b10e5d2` Val) | Pass (39/39) | Local CTest run |
| 2026-08-29 | `ctest --preset windows-x64-debug` (post `d77a89f` Abs/Sgn) | Pass (40/40) | Local CTest run |
| 2026-08-29 | `ctest --preset windows-x64-debug` (post `cd59a9d` CStr) | Pass (41/41) | Local CTest run |
| 2026-08-29 | `ctest --preset windows-x64-debug` (post `205f227` CLng) | Pass (42/42) | Local CTest run |
| 2026-08-29 | `ctest --preset windows-x64-debug` (post `11bd3f3` CBool) | Pass (43/43) | Local CTest run |
| 2026-08-29 | `ctest --preset windows-x64-debug` (post `850fd6d` CByte) | Pass (44/44) | Local CTest run |
| 2026-08-30 | `ctest --preset windows-x64-debug` (post CInt) | Pass (45/45) | Local CTest run |
| 2026-08-30 | `ctest --preset windows-x64-debug` (post `ee65241` IsNumeric) | Pass (46/46) | Local CTest run |
| 2026-08-30 | `ctest --preset windows-x64-debug` (post `ea591de` TypeName) | Pass (47/47) | Local CTest run |
| 2026-08-30 | `ctest --preset windows-x64-debug` (post `de6bc93` VarType) | Pass (48/48) | Local CTest run |
| 2026-08-30 | `ctest --preset windows-x64-debug` (post `3ff3fea` IIf) | Pass (49/49) | Local CTest run |
| 2026-08-30 | `ctest --preset windows-x64-debug` (post `ef5bea5` Choose) | Pass (50/50) | Local CTest run |
| 2026-08-30 | `ctest --preset windows-x64-debug` (post `d954272` Switch) | Pass (51/51) | Local CTest run |
| 2026-08-30 | `ctest --preset windows-x64-debug` (post `48ef2b7` Int/Fix) | Pass (52/52) | Local CTest run |
| 2026-08-30 | `ctest --preset windows-x64-debug` (post `0a60bae` AscW/ChrW) | Pass (53/53) | Local CTest run |

## Decisions and Scope Changes

| Decision or change | Authority | Impact | Reference |
| --- | --- | --- | --- |
| Continue the `Strings` (`REQ-0071`) function series as the next MP-0002 increments | Owner request ("continue working on wfc") | Adds intrinsic functions within the existing MP-0002 subset; no scope expansion | This log |
| Expose all three `VbCompareMethod` names but reject database comparison at execution | Installed VBA type-library contract plus current host boundary | Preserves source-visible values without claiming an unavailable database collation | `REQ-0167` |
| Keep `Val` in the current `Long` model and reject fractional/radix forms explicitly | Current evaluator value architecture | Prevents silent truncation or a false claim of VBA's `Double` result semantics | `REQ-0170` |
| Where VB6 `Choose`/`Switch` return `Null` (out-of-range index, no matching expression), emit an explicit diagnostic (`WFC0089`/`WFC0090`) | Current evaluator value model has no `Null`/`Variant` | Prevents a false claim of `Null` semantics; behavior tightens to an error until a `Variant` model exists | This log |
| Restrict `IsNumeric`/`VarType`/`TypeName` to the current `Long`/`Boolean`/`String` value set | Current evaluator value architecture | Reports only representable type codes/names; floating and `Null`/`Empty` classifications deferred with the numeric/Variant model | This log |

| Item | Effect | Response | Status or owner |
| --- | --- | --- | --- |
| `InStr` `Option Compare Text` case first used `expect_success` (single-statement helper) for a two-line program | One unit assertion failed (`WFC0001` — parser saw a program where a statement was expected) | Switched the multi-line case to `expect_program_success`; rebuilt and reran to green | Closed |
| Reserved `vbTextCompare` declaration test initially expected duplicate-name diagnostic `WFC0013` | One evaluator assertion failed; implementation correctly emitted the established reserved-keyword diagnostic `WFC0017` | Corrected the test expectation and reran all 36 cases successfully | Closed |
| Initial `CBool` tests concatenated Boolean results directly | Unit and CLI cases correctly emitted `WFC0020` because the current `&` contract accepts only String or Long operands | Changed unit cases to independent `Print` statements and the CLI case to explicit `CStr` conversions; reran all 43 cases successfully | Closed |
| CTest's regular-expression engine did not match the multi-line `CBool` CLI output | The CLI command produced the correct `True`/`False` lines, but its pass expression failed | Kept the CLI assertion single-line through `CStr(CBool(...))` and restored an exact output expression | Closed |

## Measurements

| Measure | Value | Source or interpretation |
| --- | ---: | --- |
| Tests before session | 30 | CTest at `89aff7f` |
| Tests after session | 35 | CTest at `1449c35` |
| CTest cases added | 5 | One integration CLI case per increment |
| New intrinsic functions | 7 | `InStr`, `StrComp`, `Replace`, `Hex`/`Hex$`, `Oct`/`Oct$`, `Str`/`Str$` |
| Functional commits pushed | 5 | `b3cfdc7`, `bbdfdf2`, `3b8ed18`, `4ed283c`, `1449c35` |
| Tests at resumed-session start | 35 | CTest at `955ed6d` |
| Tests at current checkpoint | 44 | CTest at `850fd6d` |
| Resumed-session CTest cases added | 9 | One integration CLI case per coherent increment |
| Resumed-session functional commits pushed | 9 | `451e0f6`, `fd8fb27`, `beebb9e`, `b10e5d2`, `d77a89f`, `cd59a9d`, `205f227`, `11bd3f3`, `850fd6d` |

## Resource Usage

Record token and elapsed-time telemetry when the active goal completes. The
figures are goal-level totals reported by the execution environment, not
per-commit estimates. Use `Not reported` when telemetry is unavailable rather
than estimating it.

At the owner's request, the 2026-08-30 Claude Code continuation records a
running per-increment token figure. The Claude Code environment does not expose
a live token counter to the assistant, so these per-increment figures are
assistant-side estimates (marked `est.`) rather than measured telemetry; the
authoritative goal-level total will be substituted from the environment report
when the session completes.

| Goal or work period | Tokens used | Elapsed time | Source |
| --- | ---: | ---: | --- |
| MP-0002 autonomous continuation ending at `1a7a4ec` | 291,197 | 26m 56s | Codex goal-completion report |
| CInt increment (`3315931`) | ~18,000 est. | Not reported | Claude Code, assistant estimate |
| IsNumeric increment (`ee65241`) | ~16,000 est. | Not reported | Claude Code, assistant estimate |
| TypeName increment (`ea591de`) | ~12,000 est. | Not reported | Claude Code, assistant estimate |
| VarType increment (`de6bc93`) | ~15,000 est. | Not reported | Claude Code, assistant estimate |
| IIf increment (`3ff3fea`) | ~14,000 est. | Not reported | Claude Code, assistant estimate |
| Choose increment (`ef5bea5`) | ~13,000 est. | Not reported | Claude Code, assistant estimate |
| Switch increment (`d954272`) | ~13,000 est. | Not reported | Claude Code, assistant estimate |
| Int/Fix increment (`48ef2b7`) | ~13,000 est. | Not reported | Claude Code, assistant estimate |
| AscW/ChrW increment (`0a60bae`) | ~11,000 est. | Not reported | Claude Code, assistant estimate |

## Preservation and Handoff

Retained evidence is the CTest output and the Git commit history on
`origin/master`. The previously pending `.gitignore` edit (ignore Aider working
files) was committed on 2026-08-30 at the owner's request (`91c1b0c`), and the
project settings now pre-authorize `git`/`gh` and general shell use. All WFC
work is committed and pushed; GitHub Actions validates the x86 and ARM64
targets.

**Deliberate scope boundaries for the next session:**

- `InStr`, `StrComp`, `Replace`, and `InStrRev` now accept their controlled
  positional comparison forms. `vbDatabaseCompare` is source-visible but remains
  an explicit unsupported execution mode outside a database host.
- `Hex`/`Oct`/`Str` operate on the `Long` value type only, consistent with the
  current MP-0002 numeric subset.
- `Val` returns `Long` in the current evaluator. Its fractional, exponent, and
  radix forms remain explicitly deferred until the necessary numeric semantics
  can be represented without truncation.

**Suggested next increments:** the remaining integer-compatible
information/conversion functions. Adding
floating-point math or completing `Val` should follow a deliberate numeric-value
architecture increment rather than extending the current `Long` variant ad hoc.

**Next responsible party:** the maintainer or a subsequent assistant session,
continuing the `Strings`/`Conversion` build-out under MP-0002.

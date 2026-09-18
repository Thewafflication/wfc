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
| 2026-08-30 #29 | Construction | Add the remaining Information predicates that are always `False` in the current scalar value model | Commit `e17d5b7` |
| 2026-08-30 #30 | Construction | Add `RGB` component validation, clamping, and VB color packing; consolidate scalar Information traceability in `REQ-0176` | Commit `9af4ef8` |
| 2026-08-30 #31 | Construction | Add the complete sixteen-entry `QBColor` palette with bounded-index diagnostics | Commit `4cd8498` |
| 2026-08-30 #32 | Construction | Add `LenB`, `AscB`, and `ChrB`/`ChrB$` over the current byte-string representation | Commit `88e5281` |
| 2026-08-30 #33 | Construction | Add `LeftB`/`RightB`/`MidB`/`InStrB` byte-measured slice and search variants under `REQ-0177` | Commit `2630483` |
| 2026-08-30 #34 | Construction | Add `StrConv` case mappings (`vbUpperCase`/`vbLowerCase`/`vbProperCase`) and source-visible `VbStrConv` constants under `REQ-0178`; reject width/kana/Unicode conversions | Commit `03e97e1` |
| 2026-08-31 #35 | Construction | Refactor constant resolution/reservation to a shared table; expose `VbTriState`/`VbCallType`/`VbFileAttribute`/`VbMsgBoxResult`/`VbDayOfWeek` under `REQ-0179` | Commit `516d51d` |
| 2026-08-31 #36 | Construction | Expose `VbMsgBoxStyle`/`VbAppWinStyle`/`VbFirstWeekOfYear`/`VbCalendar`/`VbDateTimeFormat` constants under `REQ-0179` | Commit `1af5656` |
| 2026-08-31 #37 | Construction | Add the general string constants (`vbCrLf`, `vbTab`, ...) and integer `vbObjectError` under `REQ-0179`/`REQ-0094` | Commit `cc53b1c` |
| 2026-08-31 #38 | Construction | Complete the `VbVarType` source-visible enumeration (`vbEmpty`..`vbArray`) under `REQ-0179`/`REQ-0080` | Commit `4352819` |
| 2026-08-31 #39 | Construction | Expose the `VbIMEStatus` enumeration, completing the source-visible constant contracts under `REQ-0179`/`REQ-0087` | Commit `65303ee` |
| 2026-08-31 #40 | Construction | Add `Round` over the `Long` domain (identity, non-negative digit count), completing the integer-compatible Math functions under `REQ-0180` | Commit `062e4c9` |
| 2026-08-31 #41 | Architecture | Add the `Double` numeric value type: fractional/exponent literals, widening arithmetic, `/` division, cross-type comparison, banker's-rounding conversions, and shortest-form rendering under `REQ-0181` | Commit `fdc1cd2` |
| 2026-08-31 #42 | Construction | Add `CDbl`/`CSng` conversions from Long/Double/Boolean/String, with single-precision narrowing for `CSng`, under `REQ-0182` | Commit `6a33604` |
| 2026-08-31 #43 | Construction | Add floating-point math `Sqr`/`Sin`/`Cos`/`Tan`/`Atn`/`Exp`/`Log` returning `Double`, with `Sqr`/`Log` domain checks, under `REQ-0183` | Commit `1ad9baa` |
| 2026-08-31 #44 | Construction | Extend `Val` to parse fractional/exponent prefixes returning `Double` (whole prefix stays `Long`); update `REQ-0170` | Commit `b8c3bd9` |
| 2026-08-31 #45 | Construction | Round `Double` operands of `\`/`Mod` to the nearest even `Long` (banker's rounding) before dividing; update `REQ-0181` | Commit `3478dd3` |
| 2026-08-31 #46 | Construction | Add `As Double` variables/constants with exact `Long` widening and correct `TypeName`/`VarType` introspection under `REQ-0184` | Commit `bf2ab25` |
| 2026-08-31 #47 | Construction | Add representable `#` Double and `&` Long numeric literal suffixes under `REQ-0185` | Commit `f8bb1cf` |
| 2026-08-31 #48 | Construction | Add representable `#` Double, `&` Long, and `$` String identifier declaration characters under `REQ-0186` | Commit `12403bd` |
| 2026-08-31 #49 | Construction | Extend `Round` to `Double` with banker's rounding and bounded decimal scaling under `REQ-0180` | Commit `18716a4` |
| 2026-08-31 #50 | Construction | Extend `Abs`/`Sgn`/`Int`/`Fix` across `Long` and `Double` under `REQ-0171`/`REQ-0187` | Commit `8c02b44` |
| 2026-08-31 #51 | Construction | Extend `Str`/`Str$` sign-space conversion to `Double` under `REQ-0188` | Commit `88f3cbe` |
| 2026-08-31 #52 | Construction | Extend `Hex`/`Oct` radix conversion to banker's-rounded `Double` inputs under `REQ-0189` | Commit `c72ff59` |
| 2026-08-31 #53 | Construction | Extend `CByte` to banker's-rounded `Double` inputs while retaining the 0-through-255 result range under `REQ-0175` | Commit `ace46fe` |
| 2026-08-31 #54 | Construction | Extend `IsNumeric` string classification to complete finite fractional and exponent forms under `REQ-0176` | Commit `964bc5f` |
| 2026-08-31 #55 | Construction | Extend `CBool` string conversion to complete finite fractional and exponent forms under `REQ-0174` | Commit `ef60e0c` |
| 2026-08-31 #56 | Construction | Extend `CInt`/`CLng` string conversion to finite fractional/exponent forms with banker's rounding under `REQ-0173`/`REQ-0181` | Commit `b0190d4` |
| 2026-08-31 #57 | Construction | Extend `CByte` string conversion to finite fractional/exponent forms with banker's rounding and range enforcement under `REQ-0175` | Commit `285ba0b` |
| 2026-08-31 #58 | Construction | Implement `Val` hexadecimal/octal prefixes with VBA-compatible signed 16-/32-bit interpretation under `REQ-0170` | Commit `39cc0b6` |
| 2026-08-31 #59 | Construction | Implement `CVar` as identity conversion across the current scalar `Value` model under `REQ-0190`; reconcile the `REQ-0072` implementation record | Commit `53fc1c9` |
| 2026-08-31 #60 | Construction | Extend `Hex`/`Oct` to strict finite numeric Strings with banker's rounding under `REQ-0189` | Commit `eda9641` |
| 2026-08-31 #61 | Construction | Implement `MacID` four-byte big-endian character-code packing under `REQ-0191` | Commit `f4fbf40` |
| 2026-08-31 #62 | Documentation | Restore omitted `REQ-0178` through `REQ-0183` language-index entries and reconcile the stale `REQ-0077` Math implementation record | Commit `671e4db` |
| 2026-08-31 #63 | Construction | Implement bounded `Error`/`Error$` messages, empty current-error behavior, and undefined-number fallback under `REQ-0192` | Commit `e15df3a` |
| 2026-08-31 #64 | Refactor | Centralize strict locale-independent numeric-String parsing across `IsNumeric`, `CByte`, `CDbl`/`CSng`, `CInt`, `CLng`, `CBool`, and `Hex`/`Oct` without changing their diagnostics | Commit `8fdcb53` |
| 2026-08-31 #65 | Construction | Diagnose out-of-range `CDbl` Strings and `CSng` narrowing as numeric overflow instead of malformed input or infinity under `REQ-0182` | Commit `3ff1cbc` |
| 2026-08-31 #66 | Construction | Reject non-finite transcendental results such as `Exp(1000)` with numeric overflow under `REQ-0183` | Commit `3a8c848` |
| 2026-08-31 #67 | Construction | Reject non-finite widened `Double` arithmetic results for `+`, `-`, `*`, and `/` with numeric overflow under `REQ-0181` | Commit `4008f86` |
| 2026-08-31 #68 | Construction | Add VBA-compatible `CByte(True) = 255` and `CByte(False) = 0` Boolean conversion under `REQ-0175` | Commit `2d34d09` |
| 2026-08-31 #69 | Verification | Close `REQ-0192` evidence gaps with coverage for every selected Error catalog entry and both range boundaries | Commit `bd7563e` |
| 2026-08-31 #70 | Verification | Complete `REQ-0181` non-finite arithmetic coverage with the missing widened subtraction overflow case | Commit `b9f086e` |
| 2026-09-01 #71 | Verification | Close `REQ-0175` arity evidence with zero- and two-argument `CByte` failures | Commit `692f78f` |
| 2026-09-17 #72 | Verification | Close `REQ-0182` arity evidence with zero- and two-argument `CDbl`/`CSng` failures | Commit `d5e4fe2` |
| 2026-09-17 #73 | Verification | Close `REQ-0161`/`REQ-0162`/`REQ-0165`/`REQ-0166` arity evidence for `LCase`/`UCase`/`LTrim`/`RTrim`/`Trim`/`Asc`/`Chr`/`StrReverse` | Commit `bb2e86b` |
| 2026-09-17 #74 | Verification | Close `REQ-0172`/`REQ-0173`/`REQ-0174`/`REQ-0170`/`REQ-0188`/`REQ-0189` arity evidence for `CStr`/`CLng`/`CBool`/`CInt`/`Val`/`Str`/`Hex`/`Oct` | Commit `b5c3ee1` |
| 2026-09-17 #75 | Verification | Close `REQ-0171`/`REQ-0187`/`REQ-0183` arity evidence for `Abs`/`Sgn`/`Int`/`Fix`/`Sqr`/`Sin`/`Cos`/`Tan`/`Atn`/`Exp`/`Log` | Commit `8c270b1` |
| 2026-09-17 #76 | Verification | Close `REQ-0176`/`REQ-0191` arity evidence for `IsNumeric`/`TypeName`/`VarType`/the six constant-False predicates/`MacID` | Commit `6582079` |
| 2026-09-17 #77 | Construction | Add `Format`/`Format$` with the eight named numeric styles (`General Number`, `Fixed`, `Standard`, `Percent`, `Scientific`, `Yes/No`, `True/False`, `On/Off`) over the current `Long`/`Double`/`Boolean` model under new `REQ-0193`; unit + CLI tests, README | Commit `4018bfb` |
| 2026-09-17 #78 | Reference | Probe the local VB6 6.00.8176 / `MSVBVM60.DLL` reference (`VB6.EXE` IDE, `Sub Main` writing to a file, run via F5) to determine `Rnd`'s exact generator algorithm and default seed, and to test whether `Randomize number` reproduces a fixed sequence | No commit (research; findings recorded below and in `REQ-0194`) |
| 2026-09-17 #79 | Construction | Add `Rnd`/`Randomize` under new `REQ-0194`: the reference-verified default `Rnd` sequence and `Rnd(0)` repeat-last, plus a WFC-owned deterministic reseed hash for `Randomize number`/`Rnd(negative)` and time-based entropy for argument-less `Randomize`; unit + CLI tests, README | Commit `7940625` |
| 2026-09-18 #80 | Architecture | Add the distinct `Single` numeric value type under new `REQ-0195`: `!` literal suffix/identifier character, `Dim`/`Const As Single`, exact `Long` widening, checked `Double` narrowing (assignment/Const/`CSng`), three-way `Long`/`Single`/`Double` arithmetic promotion, cross-type comparison, `TypeName`/`VarType`, and extending `CSng` (now returns genuine `Single`), `CDbl`/`CLng`/`CInt`/`CByte`/`CBool`/`CStr`/`IsNumeric`/`Abs`/`Sgn`/`Int`/`Fix`/`Round`/`Str`/`Hex`/`Oct` to accept `Single`; unit + CLI tests, README | Commit `2816d6b` |
| 2026-09-18 #81 | Architecture | Add the distinct `Currency` numeric value type under new `REQ-0196`: scaled-int64 fixed-point representation, exact (non-floating-point) `+`/`-`/`*`/`/` via a hand-rolled 128-bit multiply/divide (portable across x86/x64/ARM64, no compiler intrinsics), `@` literal suffix/identifier character, `Dim`/`Const As Currency`, four-way `Long`/`Currency`/`Single`/`Double` arithmetic promotion, new `CCur`, and extending `CDbl`/`CSng`/`CLng`/`CInt`/`CByte`/`CBool`/`CStr`/`IsNumeric`/`Abs`/`Int`/`Fix`/`Round`/`Str`/`Hex`/`Oct` to accept `Currency` (`Abs`/`Int`/`Fix`/`Round` exactly, via scaled-integer arithmetic); unit + CLI tests, README | Commit `2f3b45f` |
| 2026-09-18 #82 | Architecture | Add the scalar `Variant` foundation under new `REQ-0197` (`Empty`/`Null` literals and states, `IsNull`/`IsEmpty`, `Dim x As Variant`/bare `Dim x` retyping assignment, three-valued-logic `Null` propagation through every operator and `If`/`While`/`Do` condition, new `WFC0104`) and the distinct `Decimal` numeric value type under new `REQ-0198` (96-bit-mantissa/scale-0-28 exact arithmetic via a hand-rolled 256-bit `BigUInt`, `CDec`, new `WFC0105`, extending `Abs`/`Int`/`Fix`/`Round`/`CLng`/`CInt`/`CByte`/`CBool`/`CStr`/`CDbl`/`CSng`/`CCur`/`IsNumeric`/`Hex`/`Oct` to accept `Decimal`); a live VB6 6.00.8176 probe verified the `Null`/`Empty` semantics recorded below; unit + CLI tests, README | Commit `5d17249` |

## Reference Probe Evidence — `Rnd`/`Randomize` (increment #78)

Per `wsp/testing/test-strategy.md`'s "Reference" test layer (small VB6
programs recording observed Microsoft behavior), `Rnd`'s internal generator
algorithm and default seed are not published by Microsoft, so this increment
probed the local reference environment directly instead of guessing from
community write-ups.

**Method:** A minimal Standard EXE project (`Sub Main`, no forms) was created
under the session scratchpad, opened in the local `VB6.EXE` (6.00.8176) IDE,
and run with F5 (in-IDE execution against `MSVBVM60.DLL` 6.00.9848, per
`planning/reference-environment.md`). `Sub Main` wrote successive `Rnd`/
`Randomize` results to a text file with `Print #f`, which was then read back
and analyzed.

**Finding 1 — default sequence and generator (fully verified):** The first
`Rnd()` call from a fresh process returns `0.7055475` (the well-known VB6
fingerprint value). Brute-force search over all `2^24` possible internal
states found exactly one state, `327680`, whose LCG step
`state' = (state * 0x43FD43FD + 0xC39EC3) mod 2^24` reproduces the observed
first three values (`0.7055475`, `0.533424`, `0.5795186`) with no other
candidate state matching. `Rnd(0)` was confirmed to repeat the third value
without advancing.

**Finding 2 — `Rnd(negative)` is deterministic but its seed-derivation
formula was not solved:** `Rnd(-5)` returned `0.8383257` identically across
two separate process runs, and a subsequent `Rnd(1)` continued the chain from
that state (`step(state_for_-5)` matched the next observed value within
floating-point rounding). Twelve additional negative arguments (`-1` through
`-32768`) were probed but the exact bit-level mapping from argument to seed
was not reverse-engineered within this increment's scope.

**Finding 3 — `Randomize number` is *not* reproducible in the reference
runtime:** Three separate probes of `Randomize 1` followed by `Print Rnd`,
run with different preceding program state, returned three different values
(`0.1453565`, `0.5899273`, `0.7648737`). A fourth probe called
`Randomize 1` twice in immediate succession (same open output file, no
intervening statements) and still got two different results (`0.8569605`,
then `0.6713328`). This contradicts Microsoft's own published description of
`Randomize number` as reproducible, and means no formula can truthfully claim
to match the reference runtime's specific `Randomize(number)` sequence.

**Resulting decision:** presented to the owner as an explicit design choice
(record below); the owner selected a WFC-owned deterministic seed hash for
both `Randomize number` and `Rnd(negative)` rather than deferring them
outright, so real VB6 programs that call `Randomize N` still run under WFC
and get a usable, WFC-internally-reproducible pseudorandom sequence. See
`REQ-0194` for the resulting contract.

Probe project files were created under the session scratchpad directory (not
part of the repository) and are not retained; the derivation above is
reproducible from the recorded findings and the local reference environment
identified in `planning/reference-environment.md`.

## Reference Probe Evidence — `Null`/`Empty` semantics (increment #82)

Per `wsp/testing/test-strategy.md`'s "Reference" test layer, `Null`/`Empty`
propagation through VB6's operators is only loosely specified by Microsoft's
documentation (particularly the exact `Null`-concatenation and
`If Null Then` behavior), so this increment probed the local reference
environment directly rather than assuming from community write-ups.

**Method:** A minimal Standard EXE project (`Sub Main`, no forms) was
created under `C:\Users\jmwau\vbprobe2` (a shallow path was required — the
sandbox blocks typing a full path into a VB6 file-open dialog, and this
project's automation instead double-click-navigates folders), opened in the
local `VB6.EXE` (6.00.8176) IDE, and run with Run > Start against
`MSVBVM60.DLL`, per `planning/reference-environment.md`. Each risky
expression was evaluated by direct assignment inside `Sub Main`'s own
`On Error Resume Next` scope (an expression like `Null & "x"` raised an
unhandled error 94 when passed directly as a `Sub` argument, aborting the
run before any output was written), with `Err.Clear` between cases, and
results were written with `Print #f` to an absolute output path (a relative
`Open "out.txt" For Output` produced no discoverable file in this
environment).

**Findings (all confirmed, `Err.Number = 0` unless noted):**

| Expression | Result |
| --- | --- |
| `Null & "x"`, `"x" & Null` | `"x"` (no error) |
| `Null & Null` | Run-time error 94, "Invalid use of Null" |
| `Empty & "x"` | `"x"` |
| `TypeName(Null)` / `TypeName(Empty)` | `"Null"` / `"Empty"` |
| `IsNull(Null)` / `IsEmpty(Empty)` | `True` / `True` |
| `VarType(Null)` / `VarType(Empty)` | `1` / `0` |
| `Empty + 5` | `5` |
| `Empty = 0`, `Empty = ""` | `True`, `True` |
| `Null + 5`, `Null = 5` | Both propagate `Null`, no error |
| `CBool(Null)` | Run-time error 94 |
| `CBool(Empty)` | `False` |
| `Null And False` / `Null And True` | `False` / `Null` |
| `Null Or True` / `Null Or False` | `True` / `Null` |
| `Not Null` | `Null` |
| `If Null Then x=1 Else x=2` | Takes the `Else` branch (`x=2`); no error |

**Resulting decision:** every finding above was implemented directly (three-
valued Kleene logic for `And`/`Or`/`Not`; `Xor`/`Eqv`/`Imp` were derived
algebraically from the verified `And`/`Or`/`Not` primitives via the standard
identities, not independently probed). See `REQ-0197` for the resulting
contract.

Probe project files were created under `C:\Users\jmwau\vbprobe2` (not part
of the repository) and are not retained; the findings above are reproducible
from this record and the local reference environment identified in
`planning/reference-environment.md`.

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
| 2026-08-30 | `ctest --preset windows-x64-debug` (post `e17d5b7` Information predicates) | Pass (54/54) | Local CTest run |
| 2026-08-30 | `ctest --preset windows-x64-debug` (RGB increment) | Pass (55/55) | Local x64 CTest run |
| 2026-08-30 | `ctest --preset windows-x64-debug` (QBColor increment) | Pass (56/56) | Local x64 CTest run |
| 2026-08-30 | `ctest --preset windows-x64-debug` (post `88e5281` byte-string) | Pass (57/57) | Local x64 CTest run |
| 2026-08-30 | `ctest --preset windows-x64-debug` (post `2630483` byte-slice) | Pass (58/58) | Local x64 CTest run |
| 2026-08-30 | `ctest --preset windows-x64-debug` (post `03e97e1` StrConv) | Pass (59/59) | Local x64 CTest run |
| 2026-08-31 | `ctest --preset windows-x64-debug` (post `516d51d` VBA constants) | Pass (60/60) | Local x64 CTest run |
| 2026-08-31 | `ctest --preset windows-x64-debug` (post `1af5656` MsgBox/window/date constants) | Pass (60/60; expanded unit coverage) | Local x64 CTest run |
| 2026-08-31 | `ctest --preset windows-x64-debug` (post `cc53b1c` string constants) | Pass (61/61) | Local x64 CTest run |
| 2026-08-31 | `ctest --preset windows-x64-debug` (post `4352819` full VbVarType) | Pass (61/61; expanded unit coverage) | Local x64 CTest run |
| 2026-08-31 | `ctest --preset windows-x64-debug` (post `65303ee` VbIMEStatus) | Pass (61/61; expanded unit coverage) | Local x64 CTest run |
| 2026-08-31 | `ctest --preset windows-x64-debug` (post `062e4c9` Round) | Pass (62/62) | Local x64 CTest run |
| 2026-08-31 | `ctest --preset windows-x64-debug` (post `fdc1cd2` Double type) | Pass (63/63) | Local x64 CTest run |
| 2026-08-31 | `ctest --preset windows-x64-debug` (post `6a33604` CDbl/CSng) | Pass (64/64) | Local x64 CTest run |
| 2026-08-31 | `ctest --preset windows-x64-debug` (post `1ad9baa` float math) | Pass (65/65) | Local x64 CTest run |
| 2026-08-31 | `ctest --preset windows-x64-debug` (post `b8c3bd9` fractional Val) | Pass (65/65) | Local x64 CTest run |
| 2026-08-31 | `ctest --preset windows-x64-debug` (post `3478dd3` Double \\/Mod) | Pass (65/65) | Local x64 CTest run |

| 2026-08-31 | `ctest --preset windows-x64-debug` (post `bf2ab25` Double declarations) | Pass (66/66) | Local x64 CTest run |

| 2026-08-31 | `ctest --preset windows-x64-debug` (post `f8bb1cf` numeric literal suffixes) | Pass (67/67) | Local x64 CTest run |

| 2026-08-31 | `ctest --preset windows-x64-debug` (post `12403bd` identifier type characters) | Pass (68/68) | Local x64 CTest run |

| 2026-08-31 | `ctest --preset windows-x64-debug` (post `18716a4` fractional Round) | Pass (68/68; expanded unit/CLI coverage) | Local x64 CTest run |

| 2026-08-31 | `ctest --preset windows-x64-debug` (post `8c02b44` Double scalar math) | Pass (68/68; expanded unit/CLI coverage) | Local x64 CTest run |

| 2026-08-31 | `ctest --preset windows-x64-debug` (post `88f3cbe` Double Str) | Pass (68/68; expanded unit/CLI coverage) | Local x64 CTest run |

| 2026-08-31 | `ctest --preset windows-x64-debug` (post `c72ff59` Double radix conversion) | Pass (68/68; expanded unit/CLI coverage) | Local x64 CTest run |

| 2026-09-17 | `ctest --preset windows-x64-debug` (post CDbl/CSng arity coverage) | Pass (71/71) | Local x64 CTest run |
| 2026-09-17 | `ctest --preset windows-x64-debug` (post string-function-family arity coverage) | Pass (71/71; expanded unit coverage) | Local x64 CTest run |
| 2026-09-17 | `ctest --preset windows-x64-debug` (post conversion-function-family arity coverage) | Pass (71/71; expanded unit coverage) | Local x64 CTest run |
| 2026-09-17 | `ctest --preset windows-x64-debug` (post math-function-family arity coverage) | Pass (71/71; expanded unit coverage) | Local x64 CTest run |
| 2026-09-17 | `ctest --preset windows-x64-debug` (post information-function-family arity coverage) | Pass (71/71; expanded unit coverage) | Local x64 CTest run |
| 2026-09-17 | `ctest --preset windows-x64-debug` (post `Format`/`Format$` named styles) | Pass (72/72) | Local x64 CTest run |
| 2026-09-17 | `ctest --preset windows-x64-debug` (post `Rnd`/`Randomize`) | Pass (73/73) | Local x64 CTest run |
| 2026-09-18 | `ctest --preset windows-x64-debug` (post `Single` numeric type) | Pass (74/74) | Local x64 CTest run |
| 2026-09-18 | `ctest --preset windows-x64-debug` (post `Currency` numeric type) | Pass (75/75) | Local x64 CTest run |
| 2026-09-18 | `ctest --preset windows-x64-debug` (post scalar `Variant`/`Decimal`) | Pass (77/77) | Local x64 CTest run |

## Decisions and Scope Changes

| Decision or change | Authority | Impact | Reference |
| --- | --- | --- | --- |
| Continue the `Strings` (`REQ-0071`) function series as the next MP-0002 increments | Owner request ("continue working on wfc") | Adds intrinsic functions within the existing MP-0002 subset; no scope expansion | This log |
| Expose all three `VbCompareMethod` names but reject database comparison at execution | Installed VBA type-library contract plus current host boundary | Preserves source-visible values without claiming an unavailable database collation | `REQ-0167` |
| Keep `Val` in the current `Long` model and reject fractional/radix forms explicitly | Current evaluator value architecture | Prevents silent truncation or a false claim of VBA's `Double` result semantics | `REQ-0170` |
| Where VB6 `Choose`/`Switch` return `Null` (out-of-range index, no matching expression), emit an explicit diagnostic (`WFC0089`/`WFC0090`) | Current evaluator value model has no `Null`/`Variant` | Prevents a false claim of `Null` semantics; behavior tightens to an error until a `Variant` model exists | This log |
| Restrict `IsNumeric`/`VarType`/`TypeName` to the current representable scalar value set | Current evaluator value architecture | Reports `Long`/`Double`/`Boolean`/`String`; `Null`/`Empty` classifications remain deferred with the Variant model | `REQ-0176`, `REQ-0184` |
| Return `False` for unavailable Information categories and reject negative `RGB` components | Current evaluator value architecture and VBA `RGB` contract | Avoids inventing unrepresentable Variant states while providing deterministic color packing and clamping | `REQ-0176` |
| Define `LenB`/`AscB`/`ChrB` against WFC's stored byte sequence | Current evaluator String architecture | Adds deterministic byte operations without claiming DBCS or BSTR-layout equivalence | `REQ-0177` |
| Implement only `Format`'s eight named numeric styles this increment; report `WFC0102` for any custom picture string or deferred named style instead of attempting a partial parser | Custom VBA picture strings (`0`/`#`/`,`/`.`/`%`/`E+`/quoted literals/multi-section `;`) need positional literal-character handling that a first increment should not approximate | Delivers the common named-style cases now; a wrong "close enough" custom-format renderer would be a worse outcome than a clear not-yet-supported diagnostic | `REQ-0193` |
| Use a WFC-owned deterministic seed hash for `Randomize number` and `Rnd(negative)`, rather than deferring them, even though it does not reproduce the reference VB6 runtime's specific per-seed sequence | Owner decision after a local VB6 6.00.8176 probe found `Randomize number` is itself not reproducible in the reference runtime (see Reference Probe Evidence above), so no formula could truthfully claim to match it | Real VB6 programs calling `Randomize N` still run under WFC with a usable, WFC-internally-reproducible sequence, instead of failing outright | `REQ-0194` |
| Sequence `Single`/`Currency`/`Decimal` as `Single` first (own full increment), deferring `Currency` and `Decimal` | Owner decision. `Decimal` is only reachable through a `Variant` (`CDec`) in real VBA, and this evaluator has no `Variant` type yet, so a declarable `Decimal` is architecturally blocked; `Currency` is a wholly different fixed-point 64-bit representation, not a narrower float, so it is a separate effort from `Single` | Delivers a complete, working `Single` foundation now rather than three simultaneous partial type systems | `REQ-0195` |
| Add checked `Double`-to-`Single` narrowing for assignment and `Const` initializers (not only the reverse widening direction) | Without it, the common case `Dim x As Single: x = 2.5` (a bare, unsuffixed literal) would fail with a type mismatch, since a plain decimal literal is `Double`; that would make ordinary `Single` declarations feel broken | Matches `CSng`'s existing narrow-with-overflow-check contract; `WFC0009` reports a narrowing result outside the finite `Single` range | `REQ-0195` |
| Leave `Rnd`'s return type as `Double` rather than switching it to genuine `Single` now that `Single` exists | `REQ-0194`'s already-shipped tests assert exact `Double`-precision rendered strings for the verified default sequence; changing the return type would change those observable values and was not part of this increment's requested scope | Documented as an explicit deferred item in `REQ-0195`'s Scope rather than a silent gap | `REQ-0195` |
| Implement `Currency` `*`/`/` with an exact, hand-rolled 128-bit multiply/divide instead of a `double` intermediate or a compiler intrinsic | `Currency`'s whole purpose is exact decimal money math; a `double` intermediate would silently reintroduce the binary-floating-point rounding `Currency` exists to avoid, and MSVC's `_mul128`/`_umul128` intrinsics are x64-only, which would break the project's x86/ARM64 targets | A portable ~130-line grade-school 128-bit multiply and binary long-division, built from ordinary `std::uint64_t` arithmetic, gives exact results on every target architecture | `REQ-0196` |
| Bound `Currency` literal parsing to a single magnitude ceiling (`int64_max`, i.e. 922337203685477.5807) for both signs, rather than allowing the most-negative literal's one-tick-wider range (...5808) | The asymmetry only matters at one exact boundary value; a uniform bound keeps the literal parser simple and its overflow diagnostic easy to reason about | The one-tick gap at the extreme negative boundary is reachable via subtraction instead (confirmed by a passing overflow test at that exact boundary) | `REQ-0196` |
| Scope `Variant` to "scalar only": free retyping across `Empty`/`Null`/`Boolean`/`Long`/`Single`/`Currency`/`Double`/`Decimal`/`String`, explicitly excluding arrays, object references, late binding, and `CVErr`/error-value Variants | Owner decision (`AskUserQuestion`), given as a two-part scoping choice alongside the `Decimal` scope below | Delivers a complete, working scalar-retyping/`Null`/`Empty` foundation now rather than a partial array/object model; `IsArray`/`IsObject`/`IsError`/`IsMissing` stay hardcoded `False`, matching `REQ-0176`'s existing precedent | `REQ-0197` |
| Implement `Decimal` as a real 96-bit-mantissa, variable-scale (0-28) exact type (a hand-rolled 256-bit `BigUInt` with multiply/binary-long-division), not an alias for `Double` or `Currency` | Owner decision (`AskUserQuestion`): "Full exact Decimal" over a simpler narrower option | Matches COM's actual `DECIMAL` layout exactly and gives genuinely exact arithmetic (verified with a multiplication whose exact intermediate product exceeds 64 bits), at the cost of a larger, hand-rolled big-integer implementation | `REQ-0198` |
| Correct an initial scoping assumption that `Dim`/`Const As Decimal` needed support | Discovered mid-implementation: real VB6 does not accept `Dim x As Decimal` at all — `Decimal` is reachable only via `CDec` into a `Variant` | No `Dim`/`Const As Decimal` parsing was added; this matches the reference language exactly rather than adding an unsupported syntax extension | `REQ-0198` |
| Place `Single` below `Decimal` in the `NumericCategory` promotion order as a reasoned-but-unverified choice | The local computer-use screenshot tool became unavailable mid-session, blocking a live VB6 probe of the `Decimal`-vs-`Single` promotion order specifically; `Decimal`-above-`Currency` and `Double`-above-everything remain independently justified without a live probe | Flagged explicitly in the `NumericCategory` code comment and in `REQ-0198`'s Scope, rather than silently asserting an unverified promotion rule | `REQ-0198` |

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
| Tests at current checkpoint | 66 | Local x64 CTest after `bf2ab25` |
| Tests at current checkpoint | 67 | Local x64 CTest after `f8bb1cf` |
| Tests at current checkpoint | 68 | Local x64 CTest after `12403bd` |

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
| Information predicates and RGB continuation | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| QBColor continuation | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Byte-string continuation | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Byte-slice increment (`2630483`) | ~14,000 est. | Not reported | Claude Code, assistant estimate |
| StrConv increment (`03e97e1`) | ~20,000 est. | Not reported | Claude Code, assistant estimate |
| VBA constant enumerations (`516d51d`) | ~26,000 est. | Not reported | Claude Code, assistant estimate |
| MsgBox/window/date constants (`1af5656`) | ~16,000 est. | Not reported | Claude Code, assistant estimate |
| General string constants (`cc53b1c`) | ~16,000 est. | Not reported | Claude Code, assistant estimate |
| Full VbVarType enumeration (`4352819`) | ~12,000 est. | Not reported | Claude Code, assistant estimate |
| VbIMEStatus enumeration (`65303ee`) | ~12,000 est. | Not reported | Claude Code, assistant estimate |
| Round function (`062e4c9`) | ~14,000 est. | Not reported | Claude Code, assistant estimate |
| Double numeric type foundation (`fdc1cd2`) | ~48,000 est. | Not reported | Claude Code, assistant estimate |
| CDbl/CSng conversions (`6a33604`) | ~18,000 est. | Not reported | Claude Code, assistant estimate |
| Floating-point math functions (`1ad9baa`) | ~20,000 est. | Not reported | Claude Code, assistant estimate |
| Fractional Val (`b8c3bd9`) | ~16,000 est. | Not reported | Claude Code, assistant estimate |
| Double integer-division rounding (`3478dd3`) | ~14,000 est. | Not reported | Claude Code, assistant estimate |
| Double declarations and introspection (`bf2ab25`) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Numeric literal suffixes (`f8bb1cf`) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Identifier type-declaration characters (`12403bd`) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Fractional Round (`18716a4`) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Double scalar math (`8c02b44`) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Double Str (`88f3cbe`) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Double radix conversion (`c72ff59`) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Double CByte conversion (increment #53) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Fractional IsNumeric strings (increment #54) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Fractional CBool strings (increment #55) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Fractional CInt/CLng strings (increment #56) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Fractional CByte strings (increment #57) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Val radix prefixes (increment #58) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| CVar scalar identity (increment #59) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Hex/Oct numeric Strings (increment #60) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| MacID four-byte conversion (increment #61) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Requirement-index/API-record reconciliation (increment #62) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Error message function subset (increment #63) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Shared numeric-String parser refactor (increment #64) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| CDbl/CSng overflow closure (increment #65) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Floating-math overflow closure (increment #66) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Double arithmetic overflow closure (increment #67) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| CByte Boolean conversion (increment #68) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Error catalog traceability closure (increment #69) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Double subtraction overflow coverage (increment #70) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| CByte arity coverage (increment #71) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| CDbl/CSng arity coverage (increment #72) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| String-function-family arity coverage (increment #73) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Conversion-function-family arity coverage (increment #74) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Math-function-family arity coverage (increment #75) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Information-function-family arity coverage (increment #76) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Format named-style implementation (increment #77) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Rnd/Randomize reference probe and implementation (increments #78-#79) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Single numeric value type (increment #80) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Currency numeric value type (increment #81) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Scalar Variant and Decimal numeric value type (increment #82) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |

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
- `Val` returns `Long` for whole decimal and hexadecimal/octal prefixes and
  `Double` for fractional or exponent prefixes.
- `Hex`/`Oct` banker's-round `Double` inputs to the 32-bit `Long` domain before
  conversion. `Str` accepts both numeric types but deliberately uses invariant
  shortest-form `Double` digits rather than locale-sensitive VB formatting.
- Numeric literals support `#` (`Double`) and `&` (`Long`); identifier
  declarations/references support `#`, `&`, and `$`. `!`, `%`, and `@` remain
  deferred rather than being mapped to an introspectively incorrect type.

**Completed Double follow-ups:** declarations and introspection (`REQ-0184`),
representable literal and identifier type characters (`REQ-0185`/`REQ-0186`),
fractional `Round` (`REQ-0180`), `Abs`/`Sgn`/`Int`/`Fix`
(`REQ-0171`/`REQ-0187`), `Str` (`REQ-0188`), and `Hex`/`Oct` (`REQ-0189`) now
cover the current `Long`/`Double` model in addition to the earlier conversions,
floating-point functions, `Val`, arithmetic, and comparison work.

**`Format`/`Format$` (increment #77, `REQ-0193`):** implements the eight named
numeric styles (`General Number`, `Fixed`, `Standard`, `Percent`,
`Scientific`, `Yes/No`, `True/False`, `On/Off`) over the current
`Long`/`Double`/`Boolean` model, plus the one-argument default-rendering form.
Custom numeric picture strings (`0`, `#`, `,`, `.`, `%`, `E+`/`E-`, quoted
literal text), the `Currency` and date/time named styles, string picture
tokens, and applying a named style to a `String` expression remain deferred to
a follow-on increment; an unrecognized `Style` reports `WFC0102` rather than a
false result.

**`Rnd`/`Randomize` (increments #78-#79, `REQ-0194`):** implements the
reference-verified default `Rnd()` sequence (24-bit LCG, verified byte-for-
byte against a local VB6 6.00.8176 probe) and `Rnd(0)` repeat-last, plus a
WFC-owned deterministic reseed hash for `Randomize number` and
`Rnd(negative)` — the reference runtime itself was found not to reproduce a
fixed `Randomize(number)` sequence, so this is a documented, evidence-based
variance rather than an attempt at exact reproduction. Calling `Rnd` without
parentheses remains deferred with every other intrinsic function under the
evaluator's existing parenthesized-call-only architecture; `Rnd` returns
`Double` (no distinct `Single` type yet), matching the `CSng` precedent.

**`Single` numeric value type (increment #80, `REQ-0195`):** adds `Single` as
a fifth distinct `Value` alternative alongside `Long`/`String`/`Boolean`/
`Double`. Covers the `!` literal suffix and identifier character, `Dim`/
`Const As Single`, exact `Long` widening, checked `Double` narrowing
(assignment, `Const`, and `CSng` all reuse the same overflow-checked narrow),
`Single`-to-`Double` widening on assignment, three-way `Long`/`Single`/
`Double` arithmetic promotion (a `Double` operand always dominates), and
extends `CSng` (now returns a genuine `Single` instead of a `Double`-narrowed-
to-float value), `CDbl`, `CLng`, `CInt`, `CByte`, `CBool`, `CStr`, `IsNumeric`,
`Abs`, `Sgn`, `Int`, `Fix`, `Round`, `Str`, and `Hex`/`Oct` to accept `Single`
the same way they already accept `Double`. `Abs`/`Int`/`Fix`/`Round` preserve
`Single` in the result, matching their existing `Double`-preserving behavior.

**`Currency` numeric value type (increment #81, `REQ-0196`):** adds `Currency`
as a sixth distinct `Value` alternative: a scaled-`int64` fixed-point
representation (four decimal digits) chosen so the type's documented range
maps exactly onto `std::int64_t`'s min/max. Covers the `@` literal suffix and
identifier character (rejecting an exponent or more than four fractional
digits, or an out-of-range magnitude, all with `WFC0006`), `Dim`/
`Const As Currency`, exact `Long` widening, checked `Single`/`Double`
narrowing and `Currency`-to-`Single`/`Double` widening, and four-way
`Long`/`Currency`/`Single`/`Double` arithmetic promotion matching VB6's
`Long < Currency < Single < Double` order. `Currency`-category `+`/`-`/`*`/`/`
compute with exact scaled-integer arithmetic rather than floating point —
`+`/`-` with checked 64-bit addition/subtraction, `*`/`/` with a hand-rolled
128-bit multiply/binary-long-division (a 64x64 product, or a `value * 10000`
numerator, can each exceed 64 bits; the intermediate is exact rather than a
lossy `double` approximation, and the implementation is portable standard
C++ rather than an x64-only compiler intrinsic, since the project also
targets x86 and ARM64). Adds the new `CCur` conversion, and extends `CDbl`,
`CSng`, `CLng`, `CInt`, `CByte`, `CBool`, `CStr`, `IsNumeric`, `Abs`, `Int`,
`Fix`, `Round`, `Str`, and `Hex`/`Oct` to accept `Currency`; `Abs`/`Int`/
`Fix`/`Round` preserve it exactly, using the same scaled-integer arithmetic
as the binary operators.

**Scalar `Variant` and `Decimal` (increment #82, `REQ-0197`/`REQ-0198`):**
adds the scalar `Variant`: `Dim x As Variant`/bare `Dim x` (initialized to
`Empty`) freely retype across every representable value type on each
assignment, unlike a fixed-type declaration; `Null` and `Empty` become real,
inspectable `Value` states (new `Empty`/`Null` structs) reachable via the
`Null`/`Empty` literal keywords, with `IsNull`/`IsEmpty` split out of the
constant-`False` predicate group to inspect them. Three-valued (Kleene)
logic governs `Null`'s propagation through `+`/`-`/`*`/`/` (propagates
`Null`), every comparison (propagates `Null`), `And`/`Or`/`Not`/`Xor`/`Eqv`/
`Imp` (the standard ternary truth tables; `Xor`/`Eqv`/`Imp` derived
algebraically from the verified `And`/`Or`/`Not`), `&` concatenation (a
single `Null` operand becomes `""`; two together report new `WFC0104`), and
every `If`/`ElseIf`/`While`/`Do` condition check (`Null`/`Empty` take the
`False` branch without error, via a new shared `coerce_condition_boolean`
helper replacing five separate inline checks). `Empty` coerces to a
type-appropriate zero (`0`, `False`, `""`) wherever a definite value is
needed. Also adds the distinct `Decimal` type, reachable only through
`Variant`/`CDec` (matching real VB6 exactly — `Dim x As Decimal` is not
valid syntax): an exact 96-bit-mantissa, variable-scale (0-28) representation
matching COM's `DECIMAL`, built on a hand-rolled 256-bit `BigUInt` (add/
subtract/schoolbook-multiply/binary-long-division) since a 96x96-bit product
needs up to 192 bits before scale reduction. `+`/`-` align to the larger
scale exactly; `*` reduces scale with banker's rounding when the exact
product doesn't fit; `/` scales the numerator up to maximize quotient
precision (up to scale 28) before exact integer division. Extends `Abs`,
`Int`, `Fix`, `Round`, `CLng`, `CInt`, `CByte`, `CBool`, `CStr`, `CDbl`,
`CSng`, `CCur`, `IsNumeric`, and `Hex`/`Oct` to accept `Decimal`
(`Abs`/`Int`/`Fix`/`Round`/`CStr` exactly, via mantissa arithmetic or
`render_decimal`, not a lossy `Double` intermediate). A live VB6 6.00.8176
probe verified every `Null`/`Empty` semantic implemented (see the Reference
Probe Evidence section above); the `Decimal`-vs-`Single` promotion order
could not be probed live this session (tooling became unavailable) and is a
disclosed, reasoned-but-unverified choice (see the Decisions table and
`REQ-0198`'s Scope).

**Remaining next increments:**

- literal/identifier `%` Integer form, once that distinct type exists;
- arrays, object references, late binding, and `CVErr`/error-value Variants
  (deliberately excluded from the "scalar Variant" scope; `IsArray`/
  `IsObject`/`IsError`/`IsMissing` stay hardcoded `False`);
- verifying the `Decimal`-vs-`Single` promotion order against the reference
  runtime (see the Decisions table above);
- `Rnd` returning a genuine `Single` instead of `Double`, now that `Single`
  exists (deliberately not changed this increment; see the Decisions table);
- `Format`'s `Currency` named style (needs a locale currency-symbol
  convention, a `Format`-specific design question separate from the
  `Currency` type itself) and the custom numeric picture strings and other
  deferred named styles listed above;
- calling any intrinsic function without parentheses (bare `Rnd`, etc.), which
  needs a `parse_primary` change shared across every function, not just `Rnd`.

`Date`/`Time` services and `Filter`/`Join`/`Split` (arrays/`Variant`) remain
later architecture increments.

**Arity-evidence sweep (increments #72-#76):** every intrinsic function
dispatched through the evaluator's arity-check table now has an explicit
zero-argument and (where applicable) excess-argument `WFC0072` test. This
closes the same latent evidence gap that `CByte` (#71) exposed: several
requirement records asserted `WFC0072` arity behavior, or relied on a shared
diagnostic contract that implies it, without a corresponding test. The
underlying evaluator logic (`src/evaluator.cpp`, the `valid_arity` dispatch
starting near line 2115) needed no changes; only test and requirement-record
gaps were closed.

**Next responsible party:** the maintainer or a subsequent assistant session,
continuing the `Strings`/`Conversion` build-out under MP-0002.

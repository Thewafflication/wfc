# Work Log — MP-0002 Core VB/VBA Language Execution

**Content type:** Work log (per `wsp/processes/work-log-template.md`)

**Milestone or work package:** MP-0002 — Core VB/VBA language execution. Began
as the `Strings` module (`REQ-0071`) intrinsic function build-out (hence this
file's original name, `work-log-mp-0002-strings.md`, renamed to
`work-log-mp-0002.md` once scope grew well beyond it); has since covered the
rest of MP-0002's intrinsic-function surface, the full numeric type system,
fixed-size arrays, a minimal object-reference stub, user-defined procedures,
and a minimal class-module foundation.

**Period:** 2026-08-29 through 2026-09-22 (spanning multiple sessions).

**Starting baseline:** `89aff7f` — "Pre-authorize local cmake/ctest/wfc runs".

**Author:** Claude (Overlord cross-project assistant), on behalf of the owner.

**Status:** Active

This log records the chronological execution of every session's work against
MP-0002 to date. It supplements, and does not replace, the accepted MP-0002
plan, the controlled Git history, and the retained CTest evidence.

## Contents by Theme

The increment table immediately below is strictly chronological; this index
groups the same increments by theme for faster navigation. Every increment
number links to nothing in particular (GitHub doesn't anchor table rows) —
search this file for `#<N>` to jump to a specific one, or `` `REQ-XXXX` `` for
a specific requirement.

- **Increments #1–#40** (2026-08-29 – 2026-08-30) — `Strings`/`Conversion`/
  `Information` intrinsic build-out: `InStr`, `StrComp`, `Replace`, `Hex`/
  `Oct`, `Str`, comparison methods, `InStrRev`, `Val`, `Abs`/`Sgn`, the
  `CXxx` conversion family, `IsNumeric`, `TypeName`, `VarType`, `IIf`/
  `Choose`/`Switch`, `Int`/`Fix`, `AscW`/`ChrW`, the constant-`False`
  Information predicates, `RGB`/`QBColor`, byte-string functions, `StrConv`,
  six increments of VBA constant enumerations, and `Round`.
- **Increments #41–#76** (2026-08-31 – 2026-09-17) — the `Double` numeric
  foundation and everything it touched: the `Double` type itself, `CDbl`/
  `CSng`, floating-point math functions, fractional `Val`, declarations,
  literal suffixes and identifier type characters, extending `Abs`/`Sgn`/
  `Int`/`Fix`/`Round`/`Str`/`Hex`/`Oct` to `Double`, a shared numeric-string
  parser refactor, four rounds of non-finite/overflow diagnostic closure,
  and the arity-evidence sweep (increments #72–#76) closing `WFC0072`
  coverage gaps across every intrinsic-function family.
- **Increments #77–#79** (2026-09-17) — `Format`/`Format$` (named numeric
  styles) and `Rnd`/`Randomize` (including a local VB6 6.00.8176 reference
  probe to determine the exact default generator and seed).
- **Increments #80–#83** (2026-09-18) — the remaining numeric value types:
  `Single`, `Currency` (exact scaled-integer arithmetic), scalar `Variant`
  and `Decimal` together (with a live `Null`/`Empty` semantics probe), and
  the distinct 16-bit `Integer` type.
  - See `REQ-0195`–`REQ-0199`.
- **Increment #84** (2026-09-18) — fixed-size one-dimensional arrays
  (`REQ-0201`) and a minimal object-reference stub (`REQ-0200`), the
  evaluator's first genuinely recursive `Value` variant.
- **Increment #85** (2026-09-18) — user-defined `Sub`/`Function` procedures
  (`REQ-0202`): the module/procedure two-level scope-chain refactor,
  `ByVal`/`ByRef`, recursion, and the `Call` statement.
- **Increments #86–#89** (2026-09-22) — the class-module foundation, built
  in four back-to-back increments: fields/methods/properties/`New`/`Is`
  (`REQ-0203`); `Me` and `Class_Initialize`/`Class_Terminate`
  (`REQ-0204`); unqualified sibling `Property` writes, class-typed fields/
  return types, and indexed `Property` accessors (`REQ-0205`); and
  `Optional`/`ParamArray`/`Static`/`Public`/`Private` (`REQ-0206`).
- **Increments #118–#137** (2026-09-30) — core-language breadth pass toward
  the MP-0002 exit gate: chained field writes (`REQ-0235`), `With`
  (`REQ-0236`), `Enum` (`REQ-0237`), error handling/`GoTo`/`Err`
  (`REQ-0238`), `Array`/`Split`/`Join`/`Filter` (`REQ-0239`), conditional
  compilation (`REQ-0240`), user-defined types (`REQ-0241`), the `Date` type
  and date/time functions (`REQ-0242`), `Collection` and bare-argument calls
  (`REQ-0243`), `^`/bitwise logic/`Like`/`&H` (`REQ-0244`), file I/O and
  `Print` lists (`REQ-0245`), `Mid` statement/financial/`FormatNumber`
  (`REQ-0246`), `Byte` (`REQ-0247`), multi-declarators/`End`/`GoSub`/fixed
  strings (`REQ-0248`), `.vbp`/`.bas`/`.cls` loading (`REQ-0249`),
  `TypeOf`/`Error`/`LSet` (`REQ-0250`), class array fields (`REQ-0251`),
  loop-local `GoTo` (`REQ-0252`), `Collection` default member and UDT
  `ReDim` (`REQ-0253`), `Decimal` declarations (`REQ-0254`).

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
| 2026-09-18 #83 | Architecture | Add the distinct 16-bit `Integer` numeric value type under new `REQ-0199` (VB6's `Integer`, distinct from this codebase's own `Integer` C++ alias for `Long`): `%` literal suffix/identifier character, `Dim`/`Const As Integer`, checked narrowing from `Long`/`Single`/`Currency`/`Double`, six-way `Integer < Long < Currency < Single < Decimal < Double` arithmetic promotion (exact checked 16-bit arithmetic for `Integer`+`Integer`, exact widened `Long` arithmetic for mixed `Integer`/`Long`), `CInt` now returning a genuine `Integer` instead of a `Long`-typed narrowed value, and extending `CLng`/`CByte`/`CBool`/`CStr`/`CDbl`/`CSng`/`CCur`/`CDec`/`IsNumeric`/`Abs`/`Int`/`Fix`/`Round`/`Str`/`Hex`/`Oct`/`TypeName`/`VarType` to accept `Integer`; unit + CLI tests, README | Commit `10710e5` |
| 2026-09-18 #84 | Architecture | Add fixed-size one-dimensional arrays under new `REQ-0201` (`Dim arr(n)`/`Dim arr(lo To hi) As Type`, indexed read/write with `WFC0111` bounds checking, `LBound`/`UBound`, `IsArray`, `TypeName`/`VarType`; new `ArrayValue{vector<Value>, lower_bound}` alternative, a forward-declared recursive `Value` variant) and a minimal object-reference stub under new `REQ-0200` (`Nothing` as a distinct state, `Dim x As Object`, the `Set` statement as the only legal object assignment, the `Is` operator for object identity, `IsObject`/`TypeName`/`VarType`); scoped via two `AskUserQuestion` calls ("Fixed-size 1-D arrays only" / "Minimal object stub"); unit + CLI tests, README | Commit `5ca5db4` |
| 2026-09-18 #85 | Architecture | Add user-defined `Sub`/`Function` procedures under new `REQ-0202` (module-level declarations found by a pre-scan pass so calls resolve regardless of textual order, forward reference, recursion bounded by a new `WFC0123` depth guard, `ByVal`/`ByRef` parameters with copy-back for bare-identifier `ByRef` arguments, per-call local variable scope via a new `Scope`/two-level scope-chain refactor replacing the four flat `variables_`/`constants_`/`variant_variables_`/`object_variables_` members, `Function` return via self-name assignment, `Exit Sub`/`Exit Function`, and the `Call` statement); the prerequisite the owner's "finish objects" request surfaced as missing, scoped to procedures-only (no class modules) via `AskUserQuestion`; fixed a `std::vector`-reallocation pointer-invalidation bug (switched `scopes_` to `std::deque`) that silently broke `ByRef` write-back and crashed on recursion during this increment's own testing; unit + CLI tests, README | Commit `671633b` |
| 2026-09-22 #86 | Architecture | Add a class-modules foundation under new `REQ-0203`: classes supplied as separate sources alongside the standard module (new CLI `--class <Name> <source>`, new `wfc::ClassModuleSource`/`evaluate_program` overload), field declarations, `Sub`/`Function` methods, `Property Get`/`Let`/`Set` accessors, `New`/`Dim x As New ClassName`/`Dim x As ClassName`, `.` member access, unqualified sibling-member calls (implicit `Me`), and a new `ObjectInstance` `Value` alternative (a `shared_ptr<InstanceData>` handle, `InstanceData` embedding a `Scope` for field storage) giving `Is`/`Set`/`TypeName`/`VarType`/`IsObject` real non-`Nothing` results for the first time since `REQ-0200`; scoped via three `AskUserQuestion` choices ("Separate file per class" / "Fields + methods + Property accessors" / "New only, no Class_Initialize/Terminate"); fixed a live bug found during this increment's own testing where a class method calling a sibling method (including itself, for recursion) unqualified failed with "unsupported function" because `instance_scopes_` (originally a `Scope*` stack) had no way to recover the current instance's *class* to look the sibling up in, by switching it to an `InstanceData*` stack; unit + CLI tests, README | Commit `d8d3ad6` |
| 2026-09-22 #87 | Architecture | Add the `Me` keyword and the `Class_Initialize`/`Class_Terminate` lifecycle hooks under new `REQ-0204`, closing the two gaps `REQ-0203` explicitly deferred (owner request: "do the me keyword class initailize and terminate"); `Me` gives `InstanceData` `enable_shared_from_this` so it can hand out a new `ObjectInstance` sharing the current call's own instance identity; `Class_Initialize` runs from `instantiate_class` immediately after field initialization (a plain nested call, no hazard); `Class_Terminate` deliberately does **not** hook `~InstanceData()` -- doing so would let an instance's last-reference drop fire from *inside* another container's own teardown (a `Scope`'s `variables` map destroying its `Value`s as part of `scopes_.pop_back()`, or the `Interpreter`'s own final member destruction), where reentrantly pushing a new call frame onto a mid-`pop_back()` `scopes_` is undefined behavior -- instead a new `terminate_if_last_reference`/`drain_scope_instances` pair fires it only from three explicit, always-safe points (a `Set` overwrite, a call frame's locals as the call returns, and the module scope at the end of a successful program), draining one variable at a time so two same-frame aliases of one instance still terminate it exactly once; unit + CLI tests, README | Commit `df1de48` |
| 2026-09-22 #88 | Architecture | Add three class-module refinements under new `REQ-0205`, chosen (of four offered via `AskUserQuestion`) from `REQ-0203`'s own "Remaining next increments" list: unqualified sibling `Property Let`/`Set` writes (the assignment counterpart of the existing unqualified method/`Property Get` read/call), class-typed/`As Object` fields and `Function`/`Property Get` return types (reusing `REQ-0200`'s existing `Object`-variable machinery unchanged), and indexed `Property Get`/`Let`/`Set` accessors (`Property Get`'s former zero-parameter requirement and `Property Let`/`Set`'s former exactly-one-parameter requirement both relaxed); switched `scan_classes` to a two-pass scan (register every class name, then scan every body) so a class-typed field/return type can reference a class declared later on the `--class` command line; found and fixed a real bug during this increment's own testing where an unqualified read inside a `Property Let`/`Set` body ignored its own value parameter whenever it shared the property's own name (`Property Let V(v As Long)`), because the existing unqualified-`Property-Get` fallback in `parse_primary_base` ran *before* the local-parameter lookup instead of after; unit + CLI tests, README | Commit `ec4a5b3` |
| 2026-09-22 #89 | Architecture | Add the fourth item selected alongside `REQ-0205` (of four offered via `AskUserQuestion`) under new `REQ-0206`: `Optional [= default]` parameters, `ParamArray` (collecting trailing call arguments into a fresh `ArrayValue`, reusing `REQ-0201`'s existing array type unchanged), `Static` locals (persisted on the owning `ProcedureDef` itself via a new `mutable Scope statics` member and a save/restored `current_procedure_def_` pointer, copied into/out of each call's own frame), and `Public`/`Private` class-member visibility (a bare `Dim` field now implicitly `Private`, correcting `REQ-0203`'s original "no visibility modeled" simplification; enforced per-*class*, not per-instance, via a new `member_accessible` check comparing `current_class_def()` against the target's own class) -- extended to both module-level procedures and class members; found and fixed a real, unrelated latent crash during this increment's own testing: an empty `ParamArray`'s zero-length array, indexed from inside a dry-run type-check pass over an unreached `For` loop body, called `.front()` on an empty `std::vector` in `parse_array_index`'s pre-existing `!execute_` short-circuit (never triggered before, since a `Dim`-declared array can never actually be empty); also fixed a second bug in the same session where `parse_type_keyword()` was called directly after `consume_keyword("as")` with no intervening whitespace skip in the new `ParamArray` element-type parser, unlike every other `As Type` call site; unit + CLI tests, README | Commit `8ecc856` |
| 2026-09-22 #90 | Construction | Close one of the two items the owner pointed at from `REQ-0194`/`REQ-0195`'s own "Remaining next increments" bullets: `Rnd` now returns a genuine `Single` instead of `Double`, narrowing `rnd_value`'s double-precision result to `float` only at the point of return (the generator's own arithmetic, `state / 2^24`, is exact/correctly-rounded in both precisions, so narrowing the already-computed double result is provably identical to computing it in single precision from the start); every existing `Rnd`/`Randomize` test's expected string updated to the shorter, more-precisely-fingerprint-matching `Single` rendering (e.g. `0.7055475` instead of `0.7055475115776062` -- the extra digits were always double-precision noise the reference runtime never actually produces, since real VB6 `Rnd` is genuinely `Single`); `TypeName(Rnd())` now reports `"Single"`; unit + CLI tests | Commit `6e60bcf` |
| 2026-09-22 #91 | Reference, Correction | Close the second `REQ-0194`/`REQ-0195`-pointed item, "verifying the Decimal-vs-Single promotion order," with a live VB6 6.00.8176 probe (owner: "ask for vb6 again please I was afk," after an earlier attempt was denied while the owner was away) -- and found the probe's answer was bigger than the question asked: `Decimal` dominates not only `Single` (as already coded) but also `Double`, `Currency`, and `Long` in mixed-type arithmetic (`CDec(1) + 1234567.89`, a large-magnitude `Double` literal, returns `Decimal` `1234568.89`; confirmed both operand orders and for `*`/`/` too), contradicting the existing `NumericCategory` enum's `double_precision` sitting *above* `decimal_precision` and the accompanying comment's claim that "Double-above-everything" was independently justified. Corrected the promotion order (`decimal_precision` now sorts highest) in `src/evaluator.cpp`; the existing `to_decimal` widening helper already handled every lower category generically through `as_double`, so no other logic needed to change. Updated `REQ-0198`'s Requirement/Scope text (removing the now-resolved "unverified against Single" disclosure) and added mixed-type `Decimal`-dominance unit tests; unit + CLI tests, README | Commit `fd6d271` |
| 2026-09-22 #92 | Architecture | Add dynamic arrays under new `REQ-0207`, chosen (scoped via `AskUserQuestion`, "ReDim/Preserve only") from two remaining-next-increments bullets the owner pasted verbatim: `Dim identifier()` (no bound) now declares an unallocated dynamic array instead of reporting `WFC0116`, and the new `ReDim [Preserve] identifier(<bound>)` statement allocates/reallocates it, reusing `REQ-0201`'s bound-expression grammar and `WFC0117`/`WFC0115` diagnostics; new `WFC0145` reports a `ReDim` target that is not a previously declared dynamic array (undeclared, non-array, or fixed-size); `LBound`/`UBound`/indexing on an unallocated array reuses the existing `WFC0111` (matching real VB6's identical run-time error 9 for both cases). `ArrayValue` gained `is_dynamic`/`is_allocated`/`element_type_index` fields (a type *index*, not a `Value`, since a `Value` member directly inside `ArrayValue` would make `ArrayValue` and `Value` recursively complete-type-dependent on each other, unlike the existing `std::vector<Value> elements`, which breaks that cycle through heap indirection); a new `array_element_default` free function reconstructs the zero value for a stored type index. Found and fixed three real, since-fixed crash/regression risks surfaced by dynamic arrays being able to stay genuinely empty for a program's entire lifetime (not just transiently, as an empty `ParamArray` already could): `parse_array_element_assignment` read `array.elements.front().index()` unconditionally to find the element type for its type-mismatch check; `TypeName`/`VarType` read `array->elements.front()` unconditionally for the same reason; and the `ParamArray`-construction site never set its own array's `element_type_index`, which would have made `TypeName`/`VarType` on *any* `ParamArray` (empty or not) silently report the wrong type once those `.front()` calls were replaced. All three now use `element_type_index`/`array_element_default` instead of inspecting a current element; unit + CLI tests, README | Commit `6c646a3` |
| 2026-09-23 #93 | Architecture | Add `Erase` (new `REQ-0208`) and `For Each` over an array (new `REQ-0209`), the next two items from the owner's pasted remaining-increments list after `ReDim`/`ReDim Preserve`. `Erase identifier[, identifier...]` resets a fixed-size array's elements to the declared type's default in place (bounds unchanged), or fully deallocates a dynamic array (as if never `ReDim`'d, `is_allocated = false`); new `WFC0146` reports a non-array/undeclared target. `For Each identifier In arrayExpr ... Next [identifier]` iterates a snapshot of the array's elements taken at loop entry, reusing the existing numeric `For`'s `parse_for_body`/`Exit For`/`for_depth_` machinery unchanged (only the per-iteration control-variable assignment and continuation condition differ); a `Variant` control variable retypes per element, a fixed-type one requires an exact element-type match (`WFC0016` otherwise); new `WFC0147` reports a missing `In` or a non-array collection expression. Added `each`/`in`/`erase` to the reserved-identifier list, matching the existing precedent for other contextual statement keywords (`to`/`step`). Manually verified every new path (fixed/dynamic `Erase`, `Long`-typed and `Variant`-typed `For Each`, `Exit For` inside `For Each`, a zero-iteration unallocated-array loop, and the `WFC0016`/`WFC0147` diagnostics) via `wfc --eval` before writing formal tests; unit + CLI tests, README | Commit `9c0ef3d` |
| 2026-09-23 #94 | Architecture | Add fixed-size multi-dimensional arrays under new `REQ-0210`, the fourth item from the owner's pasted remaining-increments list: `Dim identifier(b1, b2, ...) As Type` (two or more comma-separated bounds, each the same `<bound>`/`<lower> To <upper>` form as the existing 1-D grammar) declares a fixed-size N-dimensional array; `identifier(i1, i2, ...)` reads/writes one element, and `LBound`/`UBound` gained an optional 1-based dimension argument (new `WFC0148` for one out of range). `ArrayValue` gained a `dimensions` field (`vector<pair<Integer,Integer>>`, empty for an ordinary 1-D array) holding each dimension's bound, with `elements` laid out flat in row-major order (last dimension fastest, matching real VB6's `For Each` iteration order); `WFC0115` (previously an unconditional "comma means multi-dim, which is unsupported" rejection) now means "index count does not match the array's declared dimension count," since indexing genuinely supports commas. Multi-dimensional arrays are fixed-size only: `ReDim`/`ReDim Preserve` (`REQ-0207`) still reject any comma in their own bound list unchanged, so a multi-dim array's shape never changes after `Dim`. Refactored the read (`parse_array_index`) and write (`parse_array_element_assignment`) index-parsing/bounds-checking into two new shared helpers, `parse_index_list` (parses N comma-separated indices, always running regardless of `execute_` so the parser advances correctly in a dry run) and `array_flat_offset` (range-checks and flattens, skipped during a dry run) -- both call sites are now shorter than before this change, not longer, despite gaining multi-dimensional support. `Erase` (`REQ-0208`) and `For Each` (`REQ-0209`) needed zero code changes: both already operate generically on `ArrayValue`'s flat `elements`/`is_dynamic`/`is_allocated` fields regardless of dimension count, confirmed by testing both directly against a 2-D array. Manually verified every new path (declaration with default and explicit `To` bounds, indexed read/write, out-of-range index, index-count mismatch in both directions, `LBound`/`UBound` with and without a dimension argument, out-of-range dimension, `TypeName`/`VarType`/`IsArray`, `For Each` summing a 2-D array, `Erase` on a 2-D array, and `ReDim` still rejecting a multi-dim bound) via `wfc --eval` before writing formal tests; unit + CLI tests, README | Commit `e6b8cae` |
| 2026-09-23 #95 | Architecture | Add array-typed `Sub`/`Function` parameters under new `REQ-0211`, the fifth and final item from the owner's original pasted remaining-increments list: `name() As Type` declares a dimension-count-agnostic, always-`ByRef` array parameter (new `ProcedureParameter.is_array_parameter`, reusing `type_index` for the required *element* type rather than a whole-array type, since no single `Value` alternative means "array of Long"). Binding requires the call argument to already be a bare-identifier array variable of matching element type (`WFC0016` mismatch, new `WFC0149` if not a variable at all); write-back needed *zero* new code, since it reuses `REQ-0202`'s existing bare-identifier `ByRef` copy-back mechanism verbatim (`parameter.by_val` is always `false` for an array parameter, and `argument.byref_target` is always non-null by construction) -- confirmed this correctly propagates not just element writes but a `ReDim Preserve` performed *inside* the callee, since the whole `ArrayValue` (including its new size/bounds) is what gets copied back. `LBound`/`UBound`/indexed read-write/`Erase`/`For Each` all work unchanged inside the callee, and a multi-dimensional array binds the same way (the parameter declaration never fixes a dimension count). New `WFC0149` also rejects an explicit `ByVal` (real VB6 disallows it on an array parameter) and `Optional` (not implemented) at the parameter-declaration site itself, alongside a malformed `name(...)` form mirroring `ParamArray`'s own `WFC0141` pattern. Extends to class methods for free, since `scan_procedure_parameters` is already shared between module-level procedures and class members. Manually verified every new path (sum-and-double round trip showing write-back, a `ReDim Preserve` inside the callee growing the caller's array, a multi-dim array bound to a plain array parameter, element-type mismatch, `ByVal`/`Optional` rejection, and a class method's own array parameter) via `wfc --eval` before writing formal tests; unit + CLI tests, README | Commit `6d1cb99` |
| 2026-09-23 #96 | Architecture | Add `Variant`- and `Object`-element arrays under new `REQ-0212`, the second and final item from the owner's original pasted list (the one not yet picked up after increment #95): `Dim identifier(...) As Variant`/`As Object` now parses for a fixed-size, dynamic, or multi-dimensional array (previously rejected with `WFC0012`, since the `!is_array &&` guard on those two `As`-clause branches simply dropped). A `Variant` element retypes freely on plain `arr(i) = expr`, mirroring a scalar `Variant`'s existing retyping rule; an `Object` element is `Set`-only (`WFC0108` on a plain `=`), mirroring a scalar `Object`'s existing rule -- both checked directly against two new `ArrayValue` fields, `is_variant_element`/`is_object_element`, rather than adding the array's own *name* to the existing `variant_variables`/`object_variables` scope-level sets (which would have wrongly let a whole-array assignment like `arr1 = arr2` retype `arr1` away from being an array at all, since those sets exist for a whole scalar/object *variable's* own rules, not an array's per-element ones). Added a new `Set arrayName(index...) = expr` parse path to `parse_set_statement` (previously `Set` had no array-element form at all), reusing `assign_object_reference` directly against the computed element slot. Fixed a real, would-have-shipped bug surfaced by writing the ReDim-refill test first: `array_element_default` (used to seed a `ReDim`-grown slot) had no `Empty`/`Nothing` cases, so a grown `Variant`/`Object` array's new slots would have gotten a stray `Integer` `0` instead -- added both cases, unambiguous since neither collides with any fixed scalar array's element type. `TypeName`/`VarType` special-case both new array kinds to report the array's own declared kind (`"Variant()"`/`"Object()"`, `8204`/`8201`) rather than trying to read a `Variant` element's current type through the now-inapplicable `element_type_index`. Removed the two now-stale `REQ-0201` unit tests that asserted `Dim arr(3) As Variant`/`As Object` fail with `WFC0012` (that failure is exactly what this increment changes). Explicitly out of scope, disclosed in `REQ-0212`: `Class_Terminate` does not proactively drain an instance reachable only through an array element (mirrors `REQ-0204`'s own existing field-cascade gap, not independently solved here), and an array-typed `Sub`/`Function` parameter (`REQ-0211`) still requires a concrete scalar element type, not `Variant`/`Object`. Manually verified every new path (per-element retyping across three scalar types with correct per-element and array-level `TypeName`/`VarType`, `Object`-element `Set`/`Is`/plain-`=`-rejection both at module level and through a class instance, a dynamic array's `ReDim Preserve` refilling new slots with `Empty`, and a multi-dimensional `Object`-element array) via `wfc --eval` before writing formal tests; unit + CLI tests, README | Commit `9aa15b5` |
| 2026-09-23 #97 | Architecture | Add zero-argument, parenthesis-free calls under new `REQ-0213`, chosen (via `AskUserQuestion`, over dynamic multi-dim arrays / small array-polish items / `Format`'s Currency style) from the remaining-increments list left after closing both owner-pasted array bullets: `Print Rnd`, `x = NextId`, an unqualified bare sibling class-Function reference, and `Call name` (module-level or class-sibling, zero arguments) all now work. Two shared argument-parsing functions -- `parse_call_argument_list` (module/class-method calls) and `parse_function_call`'s own inline argument loop (every intrinsic) -- changed their unconditional `advance()`/error-on-missing-`(` into `if (consume('(')) { ...existing... }`, leaving `arguments` empty when no `(` follows; the callee's own pre-existing arity check then naturally reports `WFC0072` if it actually needs one or more arguments, so no new arity logic was needed. `parse_primary_base`'s bare-identifier fallback (reached only once `find_variable` and the existing sibling-Property-Get check have both found nothing) gained two new checks before its final `WFC0015` "undeclared variable": a known module-level/class-sibling `Function` name (an exact map lookup, unambiguous) and a known intrinsic function name -- the second checked by speculatively calling `parse_function_call` and inspecting whether it failed specifically with `WFC0071` "unsupported function" *before consuming any input* (the intrinsic dispatcher's own not-recognized check runs before any argument parsing), in which case the speculative error is cleared and the ordinary `WFC0015` fires instead, so a genuine typo'd variable name still gets its usual diagnostic rather than a confusing "unsupported function" one. Two test-script attempts during manual verification hit boundaries that turned out to be deliberately out of this increment's scope, not bugs: a bare `Bump` written as a full statement (no `Call`) failed, because a bare-name statement without `Call` still isn't recognized at all (only `Call name` gained the zero-arg form); and `Print c.Doubled` (dotted, no parens) for a zero-arg method failed with `WFC0135`, because `parse_member_access_after_dot`'s own no-parens branch only ever checked `Property Get`/fields, never `methods` -- both are recorded as explicit Scope exclusions rather than silently-discovered gaps. Manually verified every in-scope path (bare intrinsic matching its parenthesized rendering exactly, bare/`Call`-without-parens module-level `Function` with `Static` state advancing correctly, `Call` for a `Sub`, `WFC0122` for a bare `Sub` in an expression, `WFC0015` for a genuine typo, `WFC0072` for a required-arg intrinsic called bare both with and without a trailing argument, a bare call inside an `If` condition, and an unqualified bare sibling `Function` call from within another method reached via a parenthesized outer call) via `wfc --eval` before writing formal tests, including fixing one test that mistakenly used the MP-0001-era restricted single-`Print`-statement helper (which disables identifier resolution entirely) instead of the full-program one; unit + CLI tests, README | Commit `db72e6d` |
| 2026-09-23 #98 | Architecture | Close the "small array/class polish items" bucket from the remaining-increments list (owner: "keep working", no further scoping question asked given the bucket was already itemized): class-typed array elements (new `REQ-0214`, `Dim arr(...) As SomeClass`), `Variant`/`Object`-element array-typed parameters (new `REQ-0215`, `nums() As Variant`/`As Object`), and a fixed-scalar array return type for a `Function` (new `REQ-0216`, `Function name(...) As Type()`). Class-typed arrays reuse `REQ-0212`'s generic `As Object`-element machinery unchanged, adding one new `ArrayValue.element_class_name` field threaded into the existing `assign_object_reference`'s class-match check (previously always passed an empty string for an array element); `TypeName` renders the class's own display name plus `"()"`. Variant/Object array parameters reuse `REQ-0211`'s existing `ByRef` write-back verbatim, needing only two new `ProcedureParameter` flags (`is_variant_array_parameter`/`is_object_array_parameter`) and a widened element-kind-match check in the binding loop, since `parse_type_keyword` already parsed `Variant` (the array-parameter branch was simply *rejecting* it before) and `As Object` needed the same separate check a generic Object scalar parameter already uses. The array return type is the largest of the three: a new `ProcedureDef.return_is_array` (reusing `return_type_index` for the element type, the same convention array parameters use) makes the return-value slot start as an unallocated dynamic array (`REQ-0207`) instead of a scalar zero value, letting the body either assign a whole array to its own name or `ReDim`/`ReDim Preserve` it directly through existing, completely unmodified array machinery -- verified both forms work, including `ReDim`ing the return slot by name inside the function body itself. New `WFC0150` rejects a `Variant`/`Object`/class-typed array return type (an array return must be a fixed scalar, matching the array-parameter restriction) via a new shared `parse_function_array_return_marker` helper, applied only at the two `Function`-return-type parsing sites (module-level and class-method), deliberately not at `Property Get`'s or a class field's shared `parse_scalar_object_or_class_type` call sites, keeping `Property Get`/field array-return-types and array-typed-fields out of scope exactly as before. All three features inherit `REQ-0201`'s pre-existing, disclosed whole-array-assignment simplification (`arr1 = arr2` is a same-variant-alternative copy with no per-element-type check) rather than introducing new validation, confirmed to already exist (not a gap this increment created) by testing plain array-to-array assignment across mismatched element types directly. Manually verified every new path (class-typed array `TypeName`/`Set`/`Is`, a mismatched-class `Set` correctly rejected, plain `=` still rejected, `Variant` and `Object` array parameters each with write-back, a mismatched-element-kind array argument rejected, a `Function` building and returning an array via a local variable, a `Function` `ReDim`ing its own return name directly, a class method returning an array, and `Variant`/`Object`/malformed array-return-type rejection) via `wfc --eval` before writing formal tests; unit + CLI tests, README | Commit `eb77eda` |
| 2026-09-23 #99 | Construction | Add `Format`'s `Currency` named style under an updated `REQ-0193` (owner: "keep working," no further scoping question asked -- picked from the remaining-increments list as the smallest, non-array item, now unblocked since `REQ-0196` gave the evaluator a real `Currency` type after `REQ-0193` originally deferred this style). Reuses `render_fixed_style`'s existing grouped, two-decimal "Standard" magnitude unchanged, inserting a `$` immediately before the digits (after any leading `-`) -- `$1,234.50` / `-$1,234.50`. Since this evaluator has no locale system at all, the exact rendering (a fixed US-dollar prefix, a leading `-` for negative rather than real VB6's commonly-parenthesized locale-dependent convention) is a disclosed, reasoned choice, not independently verified against the reference runtime -- recorded explicitly in `REQ-0193`'s Scope rather than presented as a verified match. Manually verified positive, negative, zero, and case-insensitive `Style` matching (`"currency"`) via `wfc --eval` before writing formal tests, then extended the existing `TC-MP0002-format-cli` CLI test in place rather than adding a new one, since it already exercises "representative styles"; unit + CLI tests, requirement doc | Commit `7c7922f` |
| 2026-09-23 #100 | Architecture | Close the two remaining parenthesis-free-call gaps under new `REQ-0217` (owner: "keep working"): a bare `name` full statement with no `Call` keyword at all (zero arguments), and `obj.Method` with no parentheses, both as a statement (with or without `Call`) and in an expression. The bare statement form is checked in `parse_statement`'s final identifier fallback, only when the name is not a variable and a lookahead confirms nothing follows it on the line (no `=`, no `(`) -- avoiding any competition with assignment or the still-excluded `name arg1, arg2` form. `obj.Method` without parens required finding and fixing *two* separate dispatch points, not one: `parse_member_access_after_dot` (reached by `Call obj.Method` and any expression read) simply never checked `class_def.methods` in its own no-parens branch, only `Property Get`/fields -- a one-line fix once found. The fully bare `obj.Method` statement (no `Call` at all) turned out to route through a completely different function, `parse_member_assignment`, which has *no* method-calling capability whatsoever (it exists solely for `obj.Prop = expr`); fixing this needed a lookahead in `parse_assignment_or_array_element`'s own object-dot branch -- parse the member name, check it names a method and that nothing follows (mirroring the bare-statement lookahead's own shape), and only then hand off to `parse_member_access_after_dot` instead of `parse_member_assignment`. A test written against an unsupported array class field (`Dim data(5) As Long`, never valid syntax here -- REQ-0203's fields are scalar-only) produced a confusing `WFC0004` "unexpected trailing input" during manual verification; recognized as a pre-existing, unrelated scope boundary once isolated, not a regression, and the test was corrected to use a valid scalar-backed class instead. Manually verified every in-scope path (bare `Function`/`Sub` statement calls with `Static` state and result-discarding behavior confirmed, ordinary variable assignment unaffected, bare and `Call`-prefixed `obj.Method`, field assignment and indexed `Property Let` both still working unaffected, and a bare method call that actually requires an argument correctly reporting `WFC0072`) via `wfc --eval` before writing formal tests; unit + CLI tests, README | Commit `82885b6` |
| 2026-09-24 #101 | Architecture | Add `Format` custom numeric picture strings under new `REQ-0218` (owner: "keep working" -- the last remaining item on the list, `Format`'s picture-string form, since dynamic multi-dim arrays and `ReDim`-as-implicit-declaration were left for a later pass). Any `Style` that does not name one of `REQ-0193`'s reserved styles is now treated as a picture rendered character by character: `0`/`#` digit placeholders (forced-zero vs. shows-nothing), a single `.` splitting the picture into integer/fraction sections and triggering the same nearest-even rounding `Fixed`/`Standard` already use (via the same bare `std::to_chars(..., chars_format::fixed, N)` call, generalized from a hardcoded 2 digits to the picture's own fraction-placeholder count), `,` among integer-section placeholders enabling comma grouping, and every other character passed through literally at its position. The integer section matches the value's digits from the rightmost placeholder leftward, with any digits beyond the leftmost placeholder still shown in full ahead of the picture's own text (placeholders set a minimum width, never a maximum) -- this specific rule was applied literally even for a picture with *no* digit placeholders at all in its integer section (e.g. `Format(42, "hello")` renders `"42hello"`, the value's digits pushed out ahead of the whole literal string), a disclosed, reasoned interpretation rather than a verified match. This retires `WFC0102` ("Format does not yet support this Style value"), previously reported unconditionally for any unrecognized `Style` and now dead code once every string has a real rendering; the one existing test asserting it (`Format(42, "Nope")`) was removed rather than updated, since it no longer represents an error case at all. Also fixed, in passing, a wrong requirement-number citation in the `Currency` style's own comment (written as `REQ-0217` in increment #99, corrected to `REQ-0193`). Manually verified zero-padding, forced/grouped/rounded decimals, negative zero-padding, trailing optional-zero trimming (`"0.0#"`), negative-sign suppression at an all-zero result, more placeholders than digits, an empty `Style`, literal text mixed before/after digit placeholders, `Boolean` widening, a decimal-only picture, integer-only grouping, and rounding to zero fraction digits, via `wfc --eval` before writing formal tests; unit + CLI tests, requirement doc | Commit `e05fb26` |
| 2026-09-24 #102 | Construction | Add dynamic multi-dimensional arrays under new `REQ-0219` (owner: "keep working until we are out of credits" -- picked from `REQ-0207`/`REQ-0210`'s own "Remaining next increments" bullets). Combines `REQ-0207`'s `ReDim`/`ReDim Preserve` with `REQ-0210`'s multi-dimensional shape: `Dim arr(,)` (N commas, no bounds) pre-declares a dimension count of N+1 before allocation; a plain `Dim arr()` instead fixes its dimension count from its own first `ReDim`'s comma count; once fixed (either way), every later `ReDim` must supply exactly that many bounds or report `WFC0115` (repurposed again); `ReDim Preserve` on a multi-dim array may only resize the *last* dimension, every other dimension's bounds must match exactly, or the statement reports new diagnostic `WFC0151`. Generalized the existing 1-D preserve-copy algorithm (outer-size loop over every dimension but the last, absolute-index overlap on the last) rather than duplicating it, so the 1-D case is just N=1 of the same code path. New shared helper `array_expected_dimension_count()` resolves "how many dimensions does this array have or will have," covering the allocated, unallocated-pre-declared, and unallocated-unconstrained cases uniformly. Manual testing (not the unit suite) surfaced a real bug: `Erase` on a dynamic array cleared `elements` but not `dimensions`, so a subsequent index on an erased multi-dim array passed a stale per-dimension bounds check and then indexed past the end of an now-empty `std::vector` -- undefined behavior, observed as a hung `wfc.exe` process that needed `taskkill //F` to terminate, not a clean crash. Diagnosed by re-reading the `Erase`/index code path after the hang, fixed by clearing `dimensions` in `Erase`'s dynamic-array branch and applying the new shared helper at that and three other call sites (`Set arr(i) = expr`, array-element write, array-element read) that had each independently, and incorrectly, computed dimension count as always-1 for an unallocated pre-declared array. Re-ran the exact hanging repro under a `timeout` guard after the fix to confirm it now completes promptly and reports `WFC0111` instead of hanging, before writing a formal regression test for it. Also updated `REQ-0208`'s Requirement/Scope (Erase's multi-dim-array behavior, including the bug fix) and `REQ-0207`/`REQ-0210`'s Scope sections to point at this requirement instead of stating multi-dimensional arrays are dynamic-array-incompatible/fixed-size-only; unit + CLI tests, requirement doc, README | Commit `29a1905` |
| 2026-09-24 #103 | Construction | Add `Format` multi-section custom numeric pictures under new `REQ-0220` (owner: "keep working until we are out of credits" -- picked from `REQ-0218`'s own "Remaining next increments" bullet). Splits a `Style` on `;` into up to three positive/negative/zero sections, matching real VB6: two sections split positive-or-zero (first) from negative (second); three add a dedicated zero section (third), chosen ahead of the sign check whenever the value is exactly `0.0`. A negative value rendered through its own negative section is passed its magnitude, not the signed value, so `REQ-0218`'s existing single-section helper's own "negative -> leading '-'" check never fires for it -- any sign shown must come from the section's own literal characters, matching real VB6 (the section is responsible for its own sign, e.g. `"(0.00)"`'s parentheses). Manually verifying this against the canonical real-world negative-section idiom -- a parenthesized or currency-prefixed picture -- surfaced two genuine pre-existing bugs in `REQ-0218`'s single-section rendering, not merely unverified edge cases: (1) overflow digits (more digits than placeholders) were inserted ahead of the *entire* picture rather than immediately next to the leftmost placeholder, so a leading literal like `(` ended up stranded in the middle of the digits (`Format(1234, "(0)")` rendered `"123(4)"` instead of `"(1234)"`); (2) comma grouping counted every character in the rendered integer section from its end rather than only its digit positions, so a literal trailing the last placeholder (a closing `)`) miscounted the grouping entirely (`Format(-1234567, "$#,##0;($#,##0)")` rendered a garbled `"(,$12,345,67)"` instead of `"($1,234,567)"`). Fixed both by tracking, alongside the existing right-to-left digit-matching buffer, where the leftmost placeholder's own position landed (so overflow digits insert there instead of at the buffer's end) and a parallel is-this-position-a-digit marker (so grouping counts only real digits, skipping any literal wherever it sits). Updated `REQ-0218`'s Requirement/Scope to describe the corrected behavior and disclose that these two cases moved from "unverified" to "found wrong, now fixed and tested"; unit + CLI tests, requirement doc | Commit `c5f87c3` |
| 2026-09-24 #104 | Construction | Add `Format` custom picture `\` escape character under new `REQ-0221` (owner: "keep working until we are out of credits" -- picked from `REQ-0218`'s own "Remaining next increments" bullet). The character right after a `\` is now always a literal -- `\0`/`\#` never a digit placeholder, `\,` never the grouping-enable comma, `\.` never the decimal-point separator, `\;` never a `REQ-0220` section separator, and `\\` a single literal backslash -- implemented by expanding every escaped pair ahead of time into a parallel `(text, forced_literal)` representation (a new `EscapedPicture`/`parse_picture_escapes` pair) and threading `forced_literal` by index through the existing digit-placeholder/decimal-point/comma-grouping checks, which previously only ever looked at the character value itself. `REQ-0220`'s own `;` section-splitting also became escape-aware in the same change (a manual scan that copies an escaped pair through untouched instead of testing its second character as a separator), since a picture with an escaped `\;` must not be split there even though section-splitting runs before a section's own escapes are expanded. Manually verified every escape shape (`\0\0\0` with no unescaped placeholder at all, `\,` combined with real placeholders on both sides, `\;` alone and combined with a real section separator later in the same picture, `\.`, and the self-referential `\\` case) via `wfc --eval` before writing formal tests -- the `\\` case needed care to reason through by hand rather than trust ad hoc shell quoting, since Bash/Windows argv backslash handling made a literal repro unreliable; the C++ unit test (exact byte control via `\\` in a C++ string literal) was the actual verification instrument for that one case. Also updated `REQ-0218`/`REQ-0220`'s Scope sections to point at this requirement instead of listing the escape character as excluded; unit + CLI tests, requirement doc | Commit `e278494` |
| 2026-09-24 #105 | Construction | Add `Format` custom picture `"..."` quoted literal text under new `REQ-0222` (owner: "keep working until we are out of credits" -- picked from `REQ-0221`'s own Scope, which had just excluded it as "VB6's other mechanism" for the same job). Every character between a pair of `"` in a custom picture is now a plain literal, VB6's second literal-text mechanism alongside the `\` escape character, and the quote delimiters themselves never appear in the output. Implemented by extending `REQ-0221`'s `parse_picture_escapes` to also track a quote-toggle state, marking every character inside a `"..."` run `forced_literal` the same way an escaped `\X` pair already is (reusing the exact same downstream `forced_literal`-aware rendering logic with no further changes needed there), and by making the `;` section-splitting loop quote-aware the same way it is already backslash-aware, so a `;` inside a quoted run does not split the picture into `REQ-0220` sections. An unterminated quoted run (no closing `"` before the section ends) makes the rest of that section literal too, a disclosed simplification mirroring the escape character's own "trailing lone `\`" one. Manually verified quoted text before and after a digit placeholder, a quoted `;` not splitting a picture (confirmed it renders identically to the already-tested `\;` case), and quoted text combined with a real unquoted section separator selecting between a positive and negative rendering, via `wfc --eval` before writing formal tests -- since a `Style` argument is itself a VB6 string literal, every test needed VB6's own doubled-quote (`""`) escaping at the *language* level before this requirement's own picture-level `"..."` delimiting ever saw the resulting string value, which was straightforward to construct correctly once reasoned through explicitly. Also updated `REQ-0221`'s Scope to point at this requirement instead of listing quoted text as excluded; unit + CLI tests, requirement doc | Commit `25e6c63` |
| 2026-09-24 #106 | Construction | Add `Format` custom picture `%` scaling under new `REQ-0223` (owner: "keep working until we are out of credits" -- picked from `REQ-0218`'s original Scope, which had deferred both `%` and `E+`/`E-` together). An unescaped, unquoted `%` anywhere in a custom picture *section* now scales the value by 100 before any digit is matched against a placeholder -- the same scaling the named `Percent` style (`REQ-0193`) already applies -- checked after `REQ-0220`'s section split and `REQ-0221`/`REQ-0222`'s escape/quote expansion, so an escaped `\%` or a quoted `"%"` is a plain literal with no scaling, and a two-section picture can scale one section without scaling the other. The `%` character itself needed no new rendering logic at all: it was never one of this format's own special characters (`0`/`#`/`.`/`,`/`\`/`"`/`;`), so it already passed through as an ordinary literal at its own position once the underlying value was scaled -- the entire feature is "detect an unescaped `%`'s presence, then scale," with no change to how it displays. Manually verified scaling for a positive and negative value, an escaped `\%` and a quoted `"%"` both suppressing scaling, and a two-section picture scaling only its selected section, via `wfc --eval` before writing formal tests. Updated `REQ-0218`'s Scope to note `%` is no longer excluded, leaving only `E+`/`E-` scientific notation combined with a custom picture (a materially larger feature, deliberately left for a later pass) under that same bullet; unit + CLI tests, requirement doc | Commit `66a5114` |
| 2026-09-28 #107 | Construction | Add `IsMissing` for an omitted `Optional Variant` argument under new `REQ-0224` (owner: "keep working" -- picked from `REQ-0206`'s own Scope, which had left `IsMissing` hardcoded `False` but explicitly named this one case real VB6 itself distinguishes). `IsMissing(paramName)` now returns `True` only when `paramName` names a no-default `Optional Variant` parameter of the *current* procedure that the caller did not supply -- a required parameter, a non-`Variant` `Optional` parameter, a *defaulted* `Variant` `Optional` parameter (the default counts as supplied in real VB6), and any name that isn't a parameter of the current procedure at all (including at module level) all still answer the pre-existing constant `False`. The implementation needed its own argument-parsing special case, distinct from the generic `parse_expression()`-per-argument loop every other intrinsic function shares: by the time an omitted Optional argument's synthesized default value would reach that generic loop, it is a real `Value` indistinguishable from a caller-supplied one, so the only way to answer correctly is to check the argument's own *name* directly -- requiring a bare-identifier grammar for `IsMissing`'s one argument instead. `invoke_definition` now records each call's own omitted no-default Variant Optional parameter names into a new `Scope::missing_parameter_names` set at the exact moment the omission is still known (right where it already decides whether to bind a real argument or synthesize the default), and the new special-cased parsing branch looks a bare identifier up first against the current procedure's own parameter list (never accidentally matching an unrelated same-named local) before checking that set. Two of this evaluator's own three pre-existing `IsMissing` tests needed updating, not as a regression fix but because they asserted the *old* stubbed constant-`False` behavior for genuinely invalid syntax (`IsMissing(7)`, a non-identifier argument) that real `IsMissing` was never actually valid for; the two arity tests (`IsMissing()`, `IsMissing(1, 2)`) mostly still passed unchanged, except `IsMissing(1, 2)`'s own diagnostic correctly became `WFC0011` (invalid argument) rather than `WFC0072` (wrong count), since `1` was never a valid parameter reference regardless of how many arguments followed it. Manually verified the omitted/supplied cases, the non-Variant-Optional-stays-False case, and -- reasoning carefully from `REQ-0206`'s own prior documentation of the exact real-VB6 nuance rather than assuming -- the defaulted-Variant-Optional-still-False case, via `wfc --eval` before writing formal tests; unit + CLI tests, requirement doc | Commit `b04f630` |
| 2026-09-28 #108 | Construction | Add unconditional `Do...Loop` under new `REQ-0225` (owner: "keep working" -- with the obvious backlog items now all larger/riskier, dispatched an `Explore` subagent to re-scan every `requirements/language/*.md` Scope section for a small, well-bounded deferred feature not already on the work log's own list; it read all 85 files and ranked unconditional `Do...Loop` first: touches one function, no design-invariant conflicts, no exhaustive-dispatch blast radius). A bare `Do` / `Loop` pair with no `While`/`Until` on either line now repeats forever, ended only by `Exit Do`/`Exit For` (`REQ-0150`, reused unchanged) -- the same thing `Do While True` already let a caller express, just without writing a condition at all. Implemented entirely inside `parse_posttest_do_statement`: where the code previously required `While`/`Until` unconditionally after `Loop` (reporting `WFC0040` otherwise), it now also accepts a statement end there directly, skipping condition-parsing and loop continuation entirely in favor of relying only on `Exit Do`/`Exit For` (or, in a dead branch, `enclosing_execution` being false to parse the body exactly once, matching every other loop form's own dry-run convention). This correctly turned previously-invalid syntax into valid, non-terminating-unless-exited syntax -- surfaced immediately by the full local suite hanging on its very first `ctest` run after the change, since one pre-existing `WFC0040` test (`Do\nPrint "no"\nLoop`, asserting the *old* "missing condition is always an error" behavior) was now a genuine infinite loop with no `Exit Do` in its body; killed the hung `wfc_frontend_tests.exe`/`ctest.exe` processes via `taskkill //F //T`, then updated that one test to use actually-malformed trailing text after `Loop` instead, which still correctly reports `WFC0040`. Manually verified `Exit Do` from directly inside an `If` block, `Exit Do` reached from inside a nested `For` loop (crossing that loop's own boundary to end the outer unconditional one), and dead-branch dry-run parsing (no hang, no output) -- each run under a `timeout` guard given the fresh memory of the hang just found -- before writing formal tests; unit + CLI tests, requirement doc | Commit `f3630b6` |
| 2026-09-28 #109 | Construction | Add `Option Base` under new `REQ-0226` (owner: "keep working" -- second pick from the same `Explore` subagent's five-candidate report, after unconditional `Do...Loop`). `Option Base 1` now changes a *bound-less* array dimension's lower bound from `0` to `1` -- `Dim` and `ReDim` alike, every dimension of a multi-dimensional array -- following the exact placement rules `Option Explicit`/`Option Compare` (`REQ-0158`/`REQ-0159`) already established (module-level only, must precede declarations, at most once). An explicit `<lower> To <upper>` dimension stays unaffected (it always uses its own written lower bound), and a `ParamArray`'s array stays `0`-based regardless -- a real, documented VB6 exception the subagent's own report had already flagged as needing confirmation, verified here by direct manual test rather than assumed. Implemented as a single evaluator-wide `option_base_one_` bool, mirroring `option_explicit_`/`option_compare_text_`'s own existing single-flag convention (this evaluator has only one standard module plus a flat `--class` list, so per-module `Option Base` scoping was never in scope for the other two directives either), consulted at exactly the two sites that defaulted a bound-less dimension's lower bound to a hardcoded `0`: `Dim`'s own array-bound parser and `ReDim`'s separate, textually-identical bound-parsing loop -- both found via `grep` for the literal `dimension_lower = 0;` assignment rather than by re-deriving the parser's structure from scratch. New diagnostics `WFC0152` (duplicate `Option Base`) and `WFC0153` (a value other than `0`/`1`) follow the exact `WFC0067`/`WFC0070` pattern `Option Explicit`/`Option Compare` already use for the same two failure shapes. Manually verified `Option Base 1` for a 1-D array, every dimension of a multi-dimensional array, `ReDim`'s own shorthand, an explicit-bound dimension staying unaffected, a `ParamArray` staying `0`-based, the no-`Option Base`-at-all default, and both new diagnostics, via `wfc --eval` before writing formal tests; unit + CLI tests, requirement doc | Commit `027d3cc` |
| 2026-09-28 #110 | Construction | Add inferred-type constants under new `REQ-0227` (owner: "keep working" -- third and final pick from the same `Explore` subagent's five-candidate report). `Const x = 5`, with neither a type-declaration character nor an `As Type` clause, now infers its type from the initializer expression instead of requiring an explicit declared type to check against -- the real-VB6 asymmetry with a bare `Dim x` (no type at all), which instead already defaults to `Variant` in this evaluator (`parse_declaration`'s own pre-existing rule): an untyped `Const` never becomes `Variant`, it takes on whatever concrete type its initializer expression naturally produces, exactly like `REQ-0157`'s own existing typed-constant form already does for the type it's told to expect. `Const x = 5` is `Long` here, not real VB6's own `Integer` -- this evaluator's unsuffixed-integer-literal-is-`Long` convention is pre-existing and evaluator-wide (`REQ-0199`'s own Scope already discloses it), so an inferred-type constant simply inherits it rather than introducing a new divergence. Implemented by reordering `parse_constant_declaration`: when no explicit type is present, the initializer is parsed *first* and its own value's `Value::index()` becomes the constant's type directly, skipping the `coerce_numeric_value`/type-mismatch check entirely (there is nothing to check it against). The agent's own report flagged this candidate's real risk correctly -- a design decision about which type an inferred integer constant should get -- and resolved it by deferring to this evaluator's own pre-existing, already-disclosed literal-typing convention rather than inventing a new one just for `Const`. Manually verified an inferred `Long`, `String`, `Boolean` (isolated from a pre-existing, unrelated `&`-concatenation-doesn't-accept-Boolean limitation hit along the way), and `Double` constant, an inferred constant's initializer referencing a previously declared inferred constant, and the existing `WFC0064` mutable-variable-reference diagnostic still applying, via `wfc --eval` before writing formal tests; unit + CLI tests, requirement doc | Commit `40cdcbe` |
| 2026-09-28 #111 | Construction | Add class-typed and generic `Object` `Sub`/`Function`/`Property` parameters under new `REQ-0228` (owner: "keep working" -- with the aggregated backlog and the first `Explore` subagent's five candidates now exhausted, dispatched a second `Explore` subagent to specifically re-scope the remaining class-features backlog cluster into tractable sub-items; it investigated three candidates and ranked "class-typed method/property parameter" first, correctly flagging one real lifetime risk in its own report before any code was written). `As Object`/`As SomeClassName` is now accepted for *any* Sub/Function/Property parameter, generalizing what was previously accepted only for `Property Set`'s own single value parameter -- reusing the exact same `parse_scalar_object_or_class_type` resolver a class-typed field/return type already uses, and threading a specific class's name into the callee's own frame (`Scope::object_class_names`) so a `Set param = ...` inside the body is class-checked by the existing, unmodified `Set` machinery, with no new enforcement code needed there. Manual testing surfaced two real bugs the subagent's own report had anticipated in outline: (1) ByRef write-back (shared by both module-level and method calls through one `invoke_definition`) copied a parameter's final value over the caller's own variable with a plain assignment, never checking whether the value already there was an `ObjectInstance` about to lose its last reference -- unreachable before this requirement (no parameter could hold an `ObjectInstance` at all), immediately reachable once object-typed parameters existed; fixed by calling `terminate_if_last_reference` (the same helper `Set` already calls before overwriting a target) immediately before each ByRef write-back, verified as a safe no-op for the ordinary unchanged-parameter case (the callee's own frame still holds a second reference at that point, so `use_count() != 1` correctly skips termination) as well as the actual bug case (a `Set param = New X` inside the callee correctly terminates the caller's old instance once write-back completes and the caller later drops its own last reference). (2) `zero_value_for_index` (synthesizing a default for an omitted `Optional` argument with no explicit default) had no case for `Nothing`'s own `Value::index()`, silently falling through to `Value{false}` -- an omitted `Optional Object` parameter bound `Boolean False`, not `Nothing`, and then failed the object-reference type check entirely; fixed by adding the missing case, the same way every other synthesized-default type already had one. Along the way, manual testing also hit an unrelated, pre-existing bug -- `If obj Is Nothing Then ... Else <access obj.Member> End If` fails even when the `Then` branch is the one actually taken, because the `Else` branch's own `obj.Member` access is not correctly treated as dead code during dry-run parsing -- confirmed with a plain `Dim o As Object` (no parameters involved at all) to isolate it as unrelated to this requirement, then flagged via `spawn_task` for separate follow-up rather than fixed here, to keep this increment's own diff focused. Manually verified a class-typed parameter reading a field, a generic `Object` parameter accepting two different classes, a class mismatch reporting `WFC0137`, `Property Set` with a specific class-typed parameter, both bug fixes directly, and an `Optional Object` parameter (omitted and explicit `= Nothing`) binding `Nothing`, via `wfc --eval` before writing formal tests; unit + CLI tests, requirement doc | Commit `1b9628c` |
| 2026-09-28 #112 | Construction | Fix member access on `Nothing` inside a not-taken branch under new `REQ-0229` (owner: "keep working" -- the pre-existing, unrelated bug found while manually testing `REQ-0228`; rather than leave it queued as a `spawn_task` suggestion for a separate session, dismissed that task and fixed it directly here, since this session already had the exact repro and root cause loaded). `If obj Is Nothing Then ... Else <access obj.Member> End If` previously raised `WFC0106` ("Invalid use of Nothing") even for the untaken `Else` branch, because that check ran completely unconditional of `execute_` -- unlike every other value-dependent runtime check this evaluator already suppresses during a dead branch's own dry-run parse (arithmetic overflow, division by zero). Fixed at all three dotted-member-access sites that can reach a `Nothing` base (a field/`Property Get` read, a `Property Let`/field write, a `Property Set` write): when `!execute_`, each now still parses through any required `(args)`/`= expression` syntactically, so the source text and shape stay validated, but returns or assigns a placeholder (`Long` `0` for a read) instead of trying to resolve a member against a class that, for a genuinely `Nothing` base, does not exist at all -- unlike a live instance, which always has one regardless of `execute_`, which is exactly why calling through a live instance in a dead branch already worked correctly before this fix. Deliberately left `CStr(Nothing)`/`Nothing` concatenation's own `WFC0106` untouched: that is a different, always-invalid-regardless-of-live-state type mismatch (closer in kind to the pre-existing `WFC0020` "concatenation requires String or Long" check, which already fires in dead branches too), not a value-dependent runtime check the dry-run convention is meant to suppress. Manually verified all three access shapes (read, `Property Let` write, `Property Set` write) plus a method call with arguments, via `wfc --eval` before writing formal tests; unit + CLI tests, requirement doc | Commit `4e4dc67` |
| 2026-09-30 #113 | Construction | Attempt comma-separated `Next` variables (`Next j, i` closing two nested `For`/`For Each` loops with one `Next`) under a would-be `REQ-0230`, then revert it (owner: "keep working"). The closing-name handoff across loop levels (a `pending_next_variable_` member, checked at the top of `parse_for_body` before scanning for a literal `Next`, reset on each retry of the *same* loop's own iteration so it cannot leak into that loop's own next re-parse) worked correctly once debugged. What did not: this evaluator requires every statement to be followed by its own terminator, consumed by a generic "parse a statement, then consume its terminator" wrapper used pervasively (module body, procedure/class-method bodies, every `If`/`Do`/`While`/`For`/`Select Case` body); `Next j, i` is one physical line with one terminator but conceptually closes *two* statements, so the inner loop's own wrapper correctly consumes the one terminator, leaving the outer loop's own *separate* required terminator-consumption step with nothing left once real code follows on the next line -- confirmed by a minimal repro that "worked" only when `Next j, i` was the very last line of the program, because `consume_statement_end()`'s own `at_end()` check tolerates being satisfied twice, masking the bug until a real statement follows it. Fixing this correctly needs a "did a nested statement already consume the shared terminator" signal threaded through every one of those wrapper call sites, not the `For`-loop-local change attempted here. Discarded the uncommitted `src/evaluator.cpp` changes via `git checkout --` (confirmed via `git status`/`git diff --stat` that nothing else was uncommitted first) and recorded the specific obstacle in the backlog instead of shipping it incomplete or leaving no trace for a future attempt | Commit `9da00e1` (documentation only; no code committed) |
| 2026-09-30 #114 | Construction | Fix two `Static Variant` object-lifetime bugs under new `REQ-0230` (reusing the number the reverted attempt above never actually consumed in any committed file; owner: "keep working" -- found while the second `Explore` subagent, back in increment #111, scoped a possible `Static` arrays/object-typed-locals extension and flagged a real lifetime gap in the *existing* `Static Variant` + `Set` combination, independent of that unimplemented extension). A `Static Variant` local can already hold an object reference via `Set` (`REQ-0200`); this exposed two bugs neither behind any unimplemented feature: (1) the end-of-call copy-back that persists a `Static`'s final value (`definition.statics.variables[name] = ...`) overwrote the previous persistent value with a plain assignment, never checking whether it was the last reference to an `ObjectInstance` -- the same class of bug `REQ-0228` fixed for a ByRef parameter's own write-back; fixed by calling `terminate_if_last_reference` on the old persistent value first, mirroring that exact fix. (2) Program-end cleanup only ever drained the module scope, never any procedure's or class member's own persistent `statics` storage, so an instance reachable only through a `Static Variant` at program end never ran `Class_Terminate` at all; fixed by draining every procedure's and every class method's/property accessor's own `statics` alongside the module scope in the same cleanup pass. Manually traced and verified the exact termination ordering across two calls to the same `Sub` (the first call's own instance correctly persists past its own call, only terminating when the *second* call's own copy-back replaces it; the second call's own instance then only terminates at program end) before writing formal tests confirming the same sequence; unit + CLI tests, requirement doc | Commit `60ae221` |
| 2026-09-30 #115 | Construction | Add `Static` arrays and class-typed/generic `Object` locals under new `REQ-0231` (owner: "work on static first" -- the remaining half of the class-cluster backlog item `REQ-0230` had just closed the lifetime-bug half of). `Static o As Object`/`As SomeClassName` now accepts an object reference the same way a class-typed field/return/parameter already does, reusing `parse_scalar_object_or_class_type` unchanged; `REQ-0230`'s two `Class_Terminate` lifetime fixes apply automatically with no further code, since both operate on any object-holding `Static`, not specifically a `Variant`-declared one -- verified directly (a `Static` class-typed local persisting an instance across three calls, correctly cumulative, with the instance surviving each individual call). `Static name(<bounds>) As Type` adds a fixed-size array, 1-D or multi-dimensional, persisted and copied element-for-element the same way a scalar `Static`'s single value already was; unlike `Dim`, `Static` has no dynamic array form at all (real VB6 requires fixed bounds at declaration time, with no `ReDim` ever possible for one), so `Static arr()` reports `WFC0149` instead of being treated as an unallocated dynamic array, and a `Variant`/`Object` element type is rejected the same way (a `Static` array's element type must be one of the seven fixed scalar types, matching `Dim`'s own array scope before `REQ-0212` widened it). Extracted the actual bound-list grammar (an expression, an optional `To`, `Option Base`'s own default lower bound, comma-separated dimensions, REQ-0201/REQ-0210/REQ-0226) out of `Dim`'s own fixed-size array parsing into a new shared `parse_fixed_array_bounds` helper -- verified the refactor alone introduced no regressions (a full `ctest` run with no test changes at all) before adding `Static`'s own use of it, rather than debugging both changes at once. Manually verified a 1-D `Static` array's element values persisting correctly across three separate calls, a multi-dimensional `Static` array, and both new `WFC0149` rejections, via `wfc --eval` before writing formal tests. Renamed this requirement from an initial, mistakenly-non-sequential `REQ-0232` to `REQ-0231` (the correct next number after `REQ-0230`) before committing, catching the numbering slip via a `grep` sweep the same way the earlier reverted `Next`-comma-list attempt's own number was reused rather than left as a gap; unit + CLI tests, requirement doc | Commit `45365b1` |
| 2026-09-30 #116 | Construction | Add `Implements` interfaces under new `REQ-0233` (owner: "do interfaces" -- the last remaining class-cluster backlog item besides class inheritance, `CreateObject`/`GetObject`/COM interop, lazy `As New`, and `Class_Terminate` field-cascading, chosen directly rather than via an `Explore` subagent ranking). `Implements InterfaceName` is stored on `ClassDef` (an interface is just an ordinary `--class` class in real VB6, no separate keyword); a new shared `class_satisfies(actual, declared)` helper widens both `Set`'s and `REQ-0228`'s parameter-binding class-match checks to accept an instance of any class that `Implements` the declared interface, not only an exact identity match. Polymorphic dispatch is a static-declaration-driven rewrite: `parse_member_access_after_dot` gained an optional `via_interface_class` parameter, populated at exactly three call sites able to cheaply obtain the base expression's declared interface type -- `parse_primary()` (via a speculative identifier lookahead that captures and restores `offset_`, since the base there is not already a resolved variable), the `Call obj.Method(args)` path, and the bare `obj.Method` (no `Call`, no parens) path (`Call Me.Method(args)` deliberately left unthreaded, since `Me` has no "declared interface type" of its own) -- rewriting `member_name` to `InterfaceName_MemberName` whenever the live instance's own class actually implements the interface named by the reference's static declared type. Manual testing surfaced a real bug: dispatching to a `Private Sub IShape_Draw()` through a legitimately `IShape`-typed reference failed with `WFC0142`, because the pre-existing per-class `Private`-visibility check had no notion of a call being sanctioned by interface dispatch; fixed by adding a `bypass_for_interface_dispatch` parameter to `member_accessible`/`call_class_method`, threaded from `parse_member_access_after_dot`'s own `dispatched_via_interface` local, verified correctly scoped (direct access to the same method by its literal `IShape_Draw` name, and a non-interface reference's bare unprefixed method name, both still correctly fail). Also confirmed, and deliberately left unfixed as out of scope, a pre-existing unrelated gap: a bare `obj.Method()` (empty parens, no `Call`, top-level statement) reports `WFC0135` regardless of interfaces, since `REQ-0217`'s bare-statement support never covered the with-parens shape; every test here uses `Call obj.Method()` instead. Manually verified polymorphic dispatch across two implementing classes through one interface-typed variable, interface-typed parameter dispatch, the `Private`-bypass fix and its two negative cases, `Set` rejecting a non-implementing class (`WFC0137`), an unknown interface name (`WFC0134`), and `Property Get` dispatch through a second interface on a class implementing two interfaces, via `wfc --eval` before writing formal tests; confirmed `REQ-0233` was the correct next unused requirement number via a `grep -rn "REQ-023"` sweep before finalizing (following the same discipline the earlier `REQ-0231`/`REQ-0232` numbering slip established); unit + CLI tests, requirement doc, updated `REQ-0203`'s Scope/`WFC0137` description to point at this requirement instead of stating class-match is always exact identity | Commit `c4498ac` |
| 2026-09-30 #117 | Construction | Add `Class_Terminate` field-cascading under new `REQ-0234` (owner: "keep working" -- the next class-cluster backlog item after `REQ-0233`, leaving class inheritance, `CreateObject`/`GetObject`/COM interop, and lazy `As New` auto-instantiation remaining). Previously `terminate_if_last_reference` only ever checked the single top-level `ObjectInstance` it was called on; when that instance's own `Class_Terminate` ran (or even when it did not, for a class with no `Class_Terminate` at all) and the instance was then actually destroyed, any of its own fields holding the last reference to another instance lost that reference through plain C++ `shared_ptr` refcounting with no `Class_Terminate` notification ever propagating -- a real, previously-disclosed gap (`REQ-0204`'s own Scope had explicitly named it as not implemented). Fixed by having `terminate_if_last_reference` call `drain_scope_instances(instance->fields)` immediately after handling the instance's own `Class_Terminate`, reusing that existing helper completely unchanged against `InstanceData::fields` (itself a `Scope`, identical in shape to a call frame's locals) instead of writing new field-walking logic -- the cascade therefore also recurses automatically to arbitrary depth, since a cascaded field's own termination is just another call to the same `terminate_if_last_reference`. The cascade runs regardless of whether the dying instance's own class declares `Class_Terminate` (an instance's fields go out of scope along with it either way), and correctly does not terminate a field's instance early when a second reference to it survives elsewhere -- verified both by reasoning through `drain_scope_instances`'s existing "clear as it goes" ordering (already relied on by every other caller) and by direct manual test. Manually verified a single-level cascade both with and without the outer class declaring its own `Class_Terminate`, and a two-level cascade (`Top` -> `Middle` -> `Deepest`) terminating in the correct outer-to-inner order, via `wfc --eval` before writing formal tests. While constructing these manual repros, found and flagged (via `spawn_task`, not fixed here) a separate, unrelated pre-existing gap: a chained two-level field write (`o.i.tag = 9`) reports `WFC0108` instead of writing through to `tag`, so every test/repro here uses an intermediate local variable at each level instead. Updated `REQ-0204`'s Scope (the original exclusion), and `REQ-0205`/`REQ-0228`'s own Scope sections that had each reaffirmed it, to point at this requirement; unit + CLI tests, requirement doc | Commit `9ef2f61` |
| 2026-09-30 #118 | Construction | Add chained field write under `REQ-0235` (`o.i.tag = 9` now writes through an object-typed field instead of reporting `WFC0108`; closes the gap `REQ-0234` flagged); `parse_member_assignment` recurses into the field's object when a further `.member` follows; added missing `<cstring>` include in `evaluator.cpp` for GCC; unit + CLI tests, requirement record. Goal token usage / elapsed time: Not reported | Commit `a6b1620` |
| 2026-09-30 #119 | Construction | Add `With ... End With` under `REQ-0236` (hidden `with.N` object variable; `parse_identifier` yields it for a leading `.member`); unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit `3edbc10` |
| 2026-09-30 #120 | Construction | Add `Enum ... End Enum` under `REQ-0237` (members as Long module constants; Enum names accepted as Long types via pre-scan); unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit `556fdbf` |
| 2026-09-30 #121 | Construction | Add `On Error`/`Resume`/`GoTo`/labels and `Err` object under `REQ-0238` (statement-level recovery wrapper, jump sentinel caught by frame body loops); single-line `If` now accepts `Exit`/`GoTo`/`Resume`/`Err.Raise`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit `c08b507` |
| 2026-09-30 #122 | Construction | Add `Array`, `Split`, `Join`, `Filter` intrinsics under `REQ-0239` (String/Variant array results); unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit `399fa10` |
| 2026-09-30 #123 | Construction | Add `#Const`/`#If`/`#ElseIf`/`#Else`/`#End If` under `REQ-0240` as an offset-preserving source preprocessor for the main program and class modules; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit `004779b` |
| 2026-09-30 #124 | Construction | Add `Type ... End Type` under `REQ-0241` (UDTs modeled as value-semantics classes: eager instantiation, deep-copy assignment and ByVal passing, nested members); also enables `arr(i).member = v` writes for object-element arrays; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit `054a8e4` |
| 2026-09-30 #125 | Construction | Add the `Date` value type, `#...#` literals, Date arithmetic/comparison/conversion, and Now/Date/Time/Timer/Year..Second/Weekday/DateSerial/TimeSerial/DateValue/TimeValue/DateAdd/DateDiff/DatePart/IsDate/CDate/MonthName/WeekdayName/FormatDateTime plus `Format(date, pattern)` under `REQ-0242`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit `78047f5` |
| 2026-09-30 #126 | Construction | Add built-in `Collection` (VB-source class registered on demand; For Each support), parenthesis-free calls with arguments, chained `Set`, Variant-field/array `Set`; fix `Exit Function` inside `Do` hanging, nested-call declaration permission, and dead-branch `Set`/`ReDim` errors under `REQ-0243`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit `99874c0` |
| 2026-09-30 #127 | Construction | Add `^`, bitwise `And`/`Or`/`Xor`/`Eqv`/`Imp`/`Not`, `Like`, `&H`/`&O` literals, string `+`, Boolean `&`, and VB ``/`Mod` precedence under `REQ-0244`; update three tests that asserted the former limits; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit `0beeb53` |
| 2026-09-30 #128 | Construction | Add sequential file I/O (`Open`/`Close`/`Print #`/`Write #`/`Input #`/`Line Input #`, `EOF`/`LOF`/`FreeFile`/`Input()`), `Print` item lists with `;`/`,`/`Spc`/`Tab`, `Kill`/`MkDir`/`RmDir`/`FileCopy`/`Dir`/`FileLen`/`CurDir`/`Environ` under `REQ-0245`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit `e818db3` |
| 2026-09-30 #129 | Construction | Wrap `fopen`/`getenv` in `fopen_s`/`_dupenv_s` on Windows so the MSVC `/WX` build does not fail on C4996 (follow-up to `REQ-0245`). Goal token usage / elapsed time: Not reported | Commit `7090550` |
| 2026-09-30 #130 | Construction | Add `Mid` statement, financial functions, `FormatNumber`/`FormatCurrency`/`FormatPercent`, `Partition`, `DoEvents`, `Debug.Print` under `REQ-0246`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit `f9b5266` |
| 2026-09-30 #131 | Construction | Add the `Byte` value type (declarations, range-checked coercion, `CByte`, `TypeName`/`VarType`, bitwise ops; arithmetic widens to Long) under `REQ-0247`; update the test that asserted `As Byte` was rejected; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit `d5724f1` |
| 2026-09-30 #132 | Construction | Add `Dim a, b` multi-declarators, module-level `Public`/`Private`/`Global`, comma-separated `Next`, `End`/`Stop`, `GoSub`/`Return`/`On..GoTo|GoSub`, `String * n`, untyped `ParamArray` under `REQ-0248`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit `3c82144` |
| 2026-09-30 #133 | Construction | Add `wfc project.vbp` / `wfc a.bas b.cls` project loading (module concatenation, class registration, header stripping, `Sub Main` startup) and accept `Option Explicit` in class modules under `REQ-0249`; CLI test with a CRLF fixture project. Goal token usage / elapsed time: Not reported | Commit `f0e8fbb` |
| 2026-09-30 #134 | Construction | Add `TypeOf .. Is`, the `Error n` statement, `LSet`/`RSet`, `Erl`, `Command` under `REQ-0250`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit `fe759c0` |
| 2026-09-30 #135 | Construction | Add dynamic, fixed, and multi-dimensional array fields in class modules (internal and `obj.field(i)` external access) under `REQ-0251`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit `6628310` |
| 2026-09-30 #136 | Construction | Take `GoTo`/`GoSub`/`Resume` jumps whose label lies inside the running `Do`/`While`/`For` body in place; accept `Case x: stmt`; accept a whole-array argument in bare-argument calls; documented as an addendum to `REQ-0248`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit `c6eed2f` |
| 2026-09-30 #137 | Construction | Make the built-in `Collection` a singly linked list (no cyclic references) so contained objects receive `Class_Terminate`; add a multi-class `Implements`/`Collection` project fixture test (`REQ-0243`, `REQ-0249`). Goal token usage / elapsed time: Not reported | Commit `2a81436` |
| 2026-09-30 #138 | Construction | Add `c(i)`/`c("key")` for `Collection` and per-slot UDT instances on `ReDim`/`ReDim Preserve` under `REQ-0253`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #139 | Construction | Accept `Decimal` in declarations with implicit numeric conversion on assignment under `REQ-0254`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #140 | Construction | Add the `Name old As new` and `ChDir` statements under `REQ-0255`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #141 | Construction | Add `Open For Binary|Random`, `Get`/`Put`/`Seek` and `Seek()` for scalar and String variables under `REQ-0256`; unit test. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #142 | Construction | Add `Attribute X.VB_UserMemId = 0` default members (`obj(i)`) for methods and indexed `Property Get`, keep the attribute in the `.cls` loader, and use it for `Collection.Item` under `REQ-0257`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #143 | Construction | Resolve `Module.Name` for standard modules declared with `Attribute VB_Name`, keeping the attribute in the project loader, under `REQ-0258`; two-module project fixture test. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #144 | Construction | Add the error-subtype Variant (`CVErr`, real `IsError`, `TypeName`/`VarType`/rendering) under `REQ-0259`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #145 | Construction | Accept `Const`, `Enum` and `Friend` in class modules (constants installed on every instance) under `REQ-0260`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #146 | Construction | Join ` _` line continuations with an offset-preserving source pass under `REQ-0261`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #147 | Construction | Raise error 20 for `Resume` outside an error handler (was an endless loop) and error 7 for `Space`/`String` counts above 256M; found by a robustness sweep; unit tests. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #148 | Construction | Add headless `MsgBox`/`InputBox`/`Beep`, in-memory settings, `CreateObject` error 429, `CVDate`, `Rate`, `MIRR`, `FileAttr`/`FileDateTime`/`GetAttr`/`SetAttr`/`Reset` under `REQ-0262`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #149 | Construction | Add `CallByName` (method/get/let/set dispatch, function and statement forms) as an addendum to `REQ-0262`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #150 | Construction | Add string formats (`@`, `&`, `<`, `>`, `!`) to `Format` under `REQ-0264`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #151 | Construction | Declare variables implicitly on assignment, and read undeclared names as Empty, unless `Option Explicit` is present anywhere in the program, under `REQ-0265`; negative tests now use `Option Explicit`. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #152 | Construction | Accept `Declare Function|Sub` (calls raise error 453) under `REQ-0266`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #153 | Construction | Accept class/UDT-typed array parameters, return UDTs from functions with copy semantics, and copy UDTs stored into Variants (Collection.Add is ByVal) under `REQ-0267`; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #154 | Construction | Draft `planning/legacy-feature-dispositions.md`: recommendations, alternatives, affected requirements and blank acceptance records for DDE, Data/DAO, OLE1, ActiveX Documents, PropertyPage and WinHelp; linked from the compatibility profile and planning index. Proposals only -- nothing accepted. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #155 | Construction | Add the self-checking `tests/corpus` programs, render Double/Single with 15/7 significant digits like VB6, fix UDT assignment in not-taken branches, and let `Err.Raise` take `vbObjectError + n` under `REQ-0268`. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #156 | Construction | Add corpus programs 13-16; convert numeric strings in arithmetic, support omitted arguments, and fix crashes found by function/operator sweeps (error Variants, Date, ordering of arrays); documented as an addendum to `REQ-0268`. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #157 | Construction | Convert between scalar types on assignment like VB6 (half-to-even rounding, Boolean/String/Date/Empty rules, catchable error 13), and require exact types for variables passed ByRef; add corpus 17; unit + CLI tests. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #158 | Construction | Allow `Dim`/`Static`/`Const` inside blocks (re-executed declarations are no-ops) and keep colon-separated statements inside a single-line `If` branch; add corpus 18-20; five obsolete negative tests now expect success; one CLI test relied on the old (wrong) colon semantics and was split across lines. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #159 | Construction | Increment: interface-typed array element dispatch (`shapes(i).Area`, bare `shapes(i).SetSize 2`); corpus 22 polymorphism. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #160 | Construction | Increment: class events (`Event`/`RaiseEvent`/`WithEvents`), weak subscriptions, ByRef event args; corpus 23. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #161 | Construction | Increment: omitted optional slots in Replace/InStr/InStrRev; corpus 24. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #162 | Construction | Increment: single-line For/While/Do loops; library argument errors map to VB error 5/9; corpus 25. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #163 | Construction | Increment: Format E+/E- scientific pictures; corpus 26. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #164 | Construction | Increment: For control variable of any numeric type, body-modified control variable; bare Proc-with-args calls inside single-line If; corpus 27. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #165 | Construction | Increment: numeric cross-type Select Case, array elements passed ByRef; corpus 28. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #166 | Construction | Increment: Boolean as -1/0 in arithmetic, Date$/Time$ as String, &H and thousands separators in numeric-string conversion; corpus 29. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #167 | Construction | Increment: placeholder operands never raise in not-taken branches, any simple statement and nested If inside single-line If, Format w/y/ww/q/ddddd/@; corpus 30. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #168 | Construction | Increment: name:=value arguments for user Sub/Function/method calls; corpus 31. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #169 | Construction | Fix: CI checkout failed because requirements/language/req-0269*.md is not a valid Windows path. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #170 | Construction | Increment: DefInt/DefStr/etc, Function with no As clause, Function Name$ suffix returns; corpus 32. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #171 | Construction | Fix: corpus 32 passed a String to a DefInt parameter. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #172 | Construction | Increment: String * n in Type, Len(udt)/Len(number), Get/Put of UDTs, array elements and arrays; corpus 33, 34. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #173 | Construction | Increment: Private x As New Cls class fields, Public Enum/Const in classes visible to other modules; corpus 35. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #174 | Construction | Increment: Resume/Resume Next inside loops, handler errors in inline If, Err.Source, more error mappings; corpus 36. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #175 | Construction | Increment: CreateObject("Scripting.Dictionary") built-in, obj(args) = value through default Property Let/Set; corpus 37, 38. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #176 | Construction | Docs: refreshed the MP-0002 status section (184 tests, 38 corpus programs, gaps and open decisions). Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #177 | Construction | Increment: numeric If/While/IIf conditions, global App object, native GetTickCount/Sleep declares, Len(Empty/Null); corpus 39. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #178 | Construction | Fix: old negative tests for numeric conditions and Declare now match the new semantics (a While 1 test had become an infinite loop). Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #179 | Construction | Increment: GetAllSettings, Shell function (synchronous), AppActivate/SendKeys no-op statements; corpus 40. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #180 | Construction | Increment: label: statement on one line, Width/Lock/Unlock; corpus 41. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #181 | Construction | Increment: module-level Property Get/Let/Set, Exit Property; corpus 42. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #182 | Construction | Increment: unsuffixed decimal integer literals beyond Long become Double; &-suffixed ones still raise. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #183 | Construction | Increment: Dim b(2) as Variant array, EnumName.Member; corpus 43. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #184 | Construction | Corpus: 44-algorithms (recursion, 2D Boolean arrays passed ByRef and replaced). Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #185 | Construction | Increment: bare Me.Method args calls; corpus 45-bank (events, errors, collections, Currency). Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #186 | Construction | Increment: ReDim obj.field(...), value semantics for arrays of UDTs; corpus 46. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #187 | Construction | Increment: Evaluation::partial_output; the CLI prints what the program printed before failing; TC-MP0002-partial-output-cli. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #188 | Construction | Increment: obj.Prop = x dispatches to Interface_Prop Property Let; corpus 47. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #189 | Construction | Increment: Dim x As New Cls creates the object on first use (VB6 semantics); corpus 48. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #190 | Construction | Increment: large reserved stack thread, recursion depth 5000 (1500 on 32-bit) with a measured-stack guard replacing the old 64-call limit; corpus 49. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #191 | Construction | Increment: Debug.Print text is kept in Evaluation::debug_output and written to stderr by the CLI; TC-MP0002-debug-print-cli. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #192 | Construction | Increment: left-associative comparison chains; corpus 50-operators. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #193 | Construction | Increment: Static locals in class methods are per object, Cls.Member for class Enums; corpus 51. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #194 | Construction | Docs: README coverage paragraph covers events, Dictionary, named arguments, DefType, deep recursion, Debug.Print. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #195 | Construction | Increment: Inc (x) / Show(a, b) as statements; corpus 52. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #196 | Construction | Increment: obj.Method (x) / obj.Method(a, b) as statements; corpus 53. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #197 | Construction | Increment: Format shows no digit for a zero integer part with # placeholders, Format(Null), FormatNumber omitted args and leading digit flag. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #198 | Construction | Corpus: 54-format-pictures. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #199 | Construction | Increment: Error$/Err.Description text for the VB6 trappable errors; corpus 55. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #200 | Construction | Increment: Evaluation::vb_error_number/description, CLI prints Run-time error N: text; TC-MP0002-runtime-error-text-cli. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #201 | Construction | Increment: ReDim As Type and lists, Variant becomes an array, free dimension count for Dim a(), String * k arrays; corpus 56. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #202 | Construction | Increment: Int16/Byte arguments widen for library routines (Mid$(s, i, i) with Integer i), ReDim declares undeclared arrays; corpus 57, 58. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #203 | Construction | Increment: literals in -32768..32767 are Integer (TypeName(5) = Integer, Integer overflow like VB6); whole-number acceptance for Tab/Spc/String * n/Len=/Sleep; REQ-0199 amended. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #204 | Construction | Docs: status section reflects module properties, Shell, literals. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #205 | Construction | Increment: CreateObject("Scripting.FileSystemObject") built-in; corpus 59. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #206 | Construction | Increment: Collection Before/After insertion, omitted arguments in parenthesis-free calls; corpus 60. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #207 | Construction | Increment: hidden-module InputB/ObjPtr/StrPtr. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #208 | Construction | Increment: Null in string functions, StrComp Null, numeric strings in Abs/Sgn/..., Chr range error mapped; corpus 61. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #209 | Construction | Increment: date functions return Null for Null, CDate overflow; corpus 62. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #210 | Construction | Increment: Array() lower bound follows Option Base 1; corpus 63. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #211 | Construction | Increment: Debug.Print inside single-line If, cached identifier-statement dispatch, user variables shadow VBA constants; corpus 64. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #212 | Construction | Perf: consume_keyword rejects on the first character inline (about 15% on a recursion-heavy program). Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #213 | Construction | Increment: CreateObject("VBScript.RegExp") with Test/Execute/Replace, MatchCollection, SubMatches; Prop(i) through a returned default member; corpus 65. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #214 | Construction | Fixed x86-only CI failure in corpus 62: Sin(1E10) differed in the last digits between x86 and x64 libm; probe now rounds to 8 places. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #215 | Construction | Fixed MSVC x86/x64 build break from the RegExp commit: std::regex::multiline is libstdc++-only; use std::regex_constants::multiline. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #216 | Construction | Made RegExp.Multiline portable: uses the STL multiline flag when present (newer MSVC/libstdc++), otherwise ^/$ match only at text ends. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #217 | Construction | Input # into Variants now yields Boolean/Date/Null for #TRUE#/#date#/#NULL# fields and Integer for small whole numbers; Write # writes dates as #yyyy-mm-dd# (time only when non-zero). Corpus 66. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #218 | Construction | Procedure headers may be followed by ':' and the body/End on the same line, in modules and classes. Corpus 67. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #219 | Construction | Corpus 67 clarified (a line-start 'Name:' is a label in VB6). Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #220 | Construction | For Each over a class exposing NewEnum (VB_UserMemId -4); Collection.[_NewEnum]; As IUnknown/IDispatch map to Object; [bracketed names] supported by a source rewrite. Corpus 68. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #221 | Construction | Rnd may be used as a statement with an argument (the Rnd -1: Randomize seed idiom). Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #222 | Construction | Shell may be called as a statement without parentheses. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #223 | Construction | Line numbers (10 stmt) act as labels and set Erl; GoTo/GoSub/On..GoTo/Resume accept line numbers. Corpus 69. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #224 | Construction | Assigning a String to a Byte array copies its UCS-2 bytes and back; StrConv(s, vbFromUnicode) returns a Byte array and StrConv(bytes, vbUnicode) a String. Corpus 70. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #225 | Construction | Date Format strings accept the AMPM token (12-hour clock with AM/PM). Corpus 71. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #226 | Construction | Collection and Scripting.Dictionary now keep their entries in a native store with a hash index of the keys: keyed Add/Item/Exists are O(1) instead of an interpreted linear scan (1000 keyed Adds: 2.3s -> 17ms). Corpus 72. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #227 | Construction | Corpus 72 reduced to 600 entries (Debug builds run the interpreter ~20x slower). Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #228 | Construction | Assignments of the form s = s & operand... extend the string in place when the operands are side-effect free; otherwise the ordinary path runs. Also concatenation appends to its left operand instead of re-copying it. Corpus 73. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #229 | Construction | Indexing the array produced by an array element or function call (jag(2)(1), Array(..)(2), Split(..)(2)) and assigning into arrays held by Variant elements (v(1)(0) = x) now work; Debug.Assert is accepted and ignored. Corpus 74. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #230 | Construction | Corpus 75: recursive-descent calculator (module-level state, Select Case, error handlers, Err.Raise). Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #231 | Construction | Class-level field declarations accept comma-separated declarators. Corpus 76 (Matrix class with indexed properties). Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #232 | Construction | Passing rec.Field, objs(i).Field or obj.Field alone as an argument now writes the parameter back (ByRef). Corpus 77. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #233 | Construction | Library-qualified calls such as VBA.Left$, VBA.Strings.UCase$, Math.Sqr and Interaction.IIf resolve to the plain functions; ChrW$ added. Corpus 78. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #234 | Construction | Numeric literals may use D as the exponent marker (1.5D-1 is a Double). Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #235 | Construction | CDate/IsDate accept month-name forms (March 5, 2024; 5-Mar-24; Mar 2024); DatePart ww honours FirstDayOfWeek/FirstWeekOfYear; DateDiff w counts weeks and ww honours FirstDayOfWeek. Corpus 79. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #236 | Construction | Mid$(...) = x is dispatched as a statement after Then. Corpus 80 (word frequency with Dictionary). Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #237 | Construction | Byte op Byte stays Byte, Byte/Integer mixes are Integer (also Mod, backslash, And/Or, unary minus); Variant-variable arithmetic that overflows promotes Integer->Long->Double instead of raising Overflow. Corpus 81. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #238 | Construction | Comparisons mixing a String and a number follow VB's Variant rules (string Variant vs typed number compares numerically, numeric Variant vs String compares as text, Variant vs Variant orders numbers before strings); a Boolean compares with numbers as -1/0. Corpus 82. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #239 | Construction | Added REQ-0281 documenting single-line procedures, numbered lines, bracket names, library qualifiers, nested array indexing, Byte/String conversion, ByRef fields, date parsing, Variant arithmetic/comparison rules, and the native Collection/Dictionary store. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #240 | Construction | Consecutive statement separators are accepted as empty statements. Corpus 83 (mixed-case keywords, tabs, continuation). Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #241 | Construction | Procedures declared Static keep every local between calls (modules and classes); Friend accepted before module-level procedures. Corpus 84. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #242 | Construction | Removed the unused 84-static-procedures.cls.cnt. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #243 | Construction | Unknown members reached at run time raise catchable error 438; Open distinguishes Path not found (76), File not found (53) and Permission denied (70); Choose returns Null out of range; ReDim with lower > upper raises error 9. Corpus 85. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #244 | Construction | Without a UI MsgBox returns the default button's result (vbYes for vbYesNo, vbAbort for vbAbortRetryIgnore, ...), honouring vbDefaultButton2/3. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #245 | Construction | Split matches its delimiter case-insensitively under Option Compare Text or vbTextCompare. Corpus 86. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #246 | Construction | b = a leaves b dynamic so Erase/ReDin still work; a ReDim Preserve that changes a non-last dimension raises catchable error 9. Corpus 87. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #247 | Construction | Chr/ChrB accept the full byte range; ChrW encodes code points above 255 as UTF-8 and AscW decodes them. Corpus 88. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #248 | Construction | Corpus 89: a .vbp with Reference/Title/version keys, two modules, and a class file with the VERSION/BEGIN header. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #249 | Construction | Format/FormatNumber-style fixed output rounds half up from the number's 15-significant-digit decimal form (Format(2.5, "0") = 3, Format(0.285, "0.00") = 0.29, Format(1234.5, "#,##0") = 1,235); Hex/Oct of a negative Integer print 16 bits. Corpus 90. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #1 | Construction | Evaluation carries error_line/error_column/error_module; the command line prints 'line N, column M' for --eval and 'file:line:col' for projects (module and class files), counting lines joined by continuations. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #2 | Construction | Early-bound uses of the built-in Scripting classes work: Dim d As New Dictionary / Scripting.Dictionary, As New FileSystemObject, As RegExp (a program-defined class of the same name wins). Corpus 91. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #3 | Construction | A program-defined Enum or class named like a VBA library (Constants, Math, ...) keeps its own member access. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #4 | Construction | Added ColorConstants (vbRed...), common VBRUN key/shift/mouse/Show constants, vbKeyA..Z/0..9, and vbUseCompareOption (-1) for the string comparison arguments. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #5 | Construction | README coverage paragraph updated for the built-in Scripting classes, numbered lines, single-line and Static procedures, NewEnum, VBA-qualified calls, and line/column diagnostics. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #6 | Construction | Exponentiation accepts Decimal operands (result Double). Corpus 92 (Decimal/Currency arithmetic). Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #7 | Construction | Declared Win32 timing and MessageBox calls work without a DLL: a 10 MHz performance counter (Currency raw count) and default-button answers. Corpus 93. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #8 | Construction | Project/.bas runs print numbers as VB6 does (sign position + trailing space) via EvaluationOptions::vb6_print_spacing; --eval and the default API stay compact. All corpus expectations regenerated (whitespace-only changes verified). REQ-0282, corpus 94. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #9 | Construction | Added evaluator unit tests covering the compact and VB6 Print number rendering. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #10 | Construction | Gave ArrayValue::element_class_name a default member initializer so aggregate initialisation no longer warns under GCC (-Werror builds). Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #11 | Construction | Public Sub Print is accepted in class modules, joining members already named after functions. Corpus 95. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #12 | Construction | Status section updated: 244 tests, 95 corpus programs, and the capabilities added by the hardening series. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #13 | Construction | Dir lists directories only with vbDirectory (including . and ..); Kill accepts * and ? masks; SetAttr toggles the read-only bit and GetAttr reports read-only/archive/directory. Corpus 96. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #14 | Construction | Untaken-branch placeholders no longer raise (chained indexing, calls through unset Object variables, Switch, For bounds, Case values); unmatched Switch yields Null; coerce_long accepts Byte/Boolean/Empty/numeric strings/Date; one-line With and Select Case headers. Corpus 97-98. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #15 | Construction | col(i).Prop = x, obj.Prop(args).member = x, obj.Prop.member = x and v(i).member = x assign into the object reached; one-line With/Select; Randomize/Rnd/Shell/For Each/logical operators tolerate placeholders in untaken branches. Corpus 99. Goal token usage / elapsed time: Not reported | Commit pending |
| 2026-09-30 #16 | Construction | Option Compare Database behaves as Binary; Option Private Module is accepted and ignored. Goal token usage / elapsed time: Not reported | Commit pending |

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

## Reference Probe Evidence — `Decimal` promotion order (increment #91)

Per `wsp/testing/test-strategy.md`'s "Reference" test layer, `REQ-0198`'s
Scope section had flagged `Decimal`'s promotion order against `Single` as a
reasoned-but-unverified choice (a prior session's probe attempt could not
complete). This increment probed the local reference environment directly.

**Method:** A Standard EXE project (`Form1`, code in `Form_Load`) was created
in the local `VB6.EXE` (6.00.8176) IDE and run with F5, per
`planning/reference-environment.md`. Each case computed a mixed-type
expression involving `CDec(1)` and wrote `TypeName(result) & " = " &
CStr(result)` to a text file with `Print #f`, read back afterward.

**Findings (all confirmed):**

| Expression | Result |
| --- | --- |
| `CDec(1) + Single(1.5)` | `Decimal = 2.5` |
| `Single(1.5) + CDec(1)` | `Decimal = 2.5` |
| `CDec(1) * Single(1.5)` | `Decimal = 1.5` |
| `Single(1.5) * CDec(1)` | `Decimal = 1.5` |
| `CDec(1) + 2.5` (Double literal) | `Decimal = 3.5` |
| `CDec(1) + 1234567.89` (large-magnitude Double literal) | `Decimal = 1234568.89` |
| `1234567.89 + CDec(1)` | `Decimal = 1234568.89` |
| `CDec(1) + Currency(2.5)` | `Decimal = 3.5` |
| `Currency(2.5) + CDec(1)` | `Decimal = 3.5` |
| `CDec(1) + Long(3)` | `Decimal = 4` |
| `CDec(1) / 3` | `Decimal = 0.3333333333333333333333333333` |

`Decimal` won every case, including against a large-magnitude `Double`
literal (ruling out a small-value fluke) and under both `+`/`*`/`/`. No case
returned `Double`, `Single`, `Currency`, or `Long`.

**Resulting decision:** `NumericCategory`'s promotion order in
`src/evaluator.cpp` was corrected so `decimal_precision` sorts above
`double_precision` (previously the reverse), matching `int16 < integer <
currency < single < double_precision < decimal_precision`. The existing
`to_decimal` widening helper inside the `decimal_precision` arithmetic branch
already converted every lower category generically via `as_double`
(previously reached only for `Int16`/`Long`/`Currency`/`Single`, now also
reached for `Double`), so no other logic changed. See `REQ-0198` for the
resulting contract.

Probe project files were created under the VB6 IDE's default project
location (not part of the repository) and are not retained; the findings
above are reproducible from this record and the local reference environment
identified in `planning/reference-environment.md`.

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
| 2026-09-18 | `ctest --preset windows-x64-debug` (post `Integer` numeric type) | Pass (78/78) | Local x64 CTest run |
| 2026-09-18 | `ctest --preset windows-x64-debug` (post fixed-size arrays and minimal object stub) | Pass (80/80) | Local x64 CTest run |
| 2026-09-18 | `ctest --preset windows-x64-debug` (post user-defined Sub/Function procedures) | Pass (81/81) | Local x64 CTest run |
| 2026-09-22 | `ctest --preset windows-x64-debug` (post class-modules foundation) | Pass (82/82) | Local x64 CTest run |
| 2026-09-22 | `ctest --preset windows-x64-debug` (post Me/Class_Initialize/Class_Terminate) | Pass (83/83) | Local x64 CTest run |
| 2026-09-22 | `ctest --preset windows-x64-debug` (post class-module refinements) | Pass (84/84) | Local x64 CTest run |
| 2026-09-22 | `ctest --preset windows-x64-debug` (post Optional/ParamArray/Static/visibility) | Pass (85/85) | Local x64 CTest run |
| 2026-09-22 | `ctest --preset windows-x64-debug` (post Rnd Single return type) | Pass (85/85) | Local x64 CTest run |
| 2026-09-22 | `ctest --preset windows-x64-debug` (post Decimal promotion-order correction) | Pass (85/85; expanded unit coverage) | Local x64 CTest run |
| 2026-09-22 | `ctest --preset windows-x64-debug` (post dynamic arrays / ReDim / ReDim Preserve) | Pass (86/86) | Local x64 CTest run |
| 2026-09-23 | `ctest --preset windows-x64-debug` (post Erase / For Each) | Pass (88/88) | Local x64 CTest run |
| 2026-09-23 | `ctest --preset windows-x64-debug` (post multi-dimensional arrays) | Pass (89/89) | Local x64 CTest run |
| 2026-09-23 | `ctest --preset windows-x64-debug` (post array-typed parameters) | Pass (90/90) | Local x64 CTest run |
| 2026-09-23 | `ctest --preset windows-x64-debug` (post Variant/Object-element arrays) | Pass (91/91) | Local x64 CTest run |
| 2026-09-23 | `ctest --preset windows-x64-debug` (post parenthesis-free niladic calls) | Pass (92/92) | Local x64 CTest run |
| 2026-09-23 | `ctest --preset windows-x64-debug` (post class-typed arrays / Variant-Object array parameters / array return type) | Pass (94/94) | Local x64 CTest run |
| 2026-09-23 | `ctest --preset windows-x64-debug` (post Format Currency style) | Pass (94/94; expanded unit coverage) | Local x64 CTest run |
| 2026-09-23 | `ctest --preset windows-x64-debug` (post bare-statement/obj.Method parenless calls) | Pass (95/95) | Local x64 CTest run |
| 2026-09-24 | `ctest --preset windows-x64-debug` (post Format custom numeric picture strings) | Pass (95/95; expanded unit coverage, one stale WFC0102 test removed) | Local x64 CTest run |
| 2026-09-24 | `ctest --preset windows-x64-debug` (post `29a1905` dynamic multi-dim arrays) | Pass (96/96; expanded unit coverage including an Erase-bug regression, one new CLI test, two stale WFC0115-on-any-comma tests removed) | Local x64 CTest run |
| 2026-09-24 | `wfc --eval` under a `timeout` guard, re-running the exact `Erase`-on-multi-dim-dynamic-array repro that previously hung the process | Now completes promptly and reports `WFC0111`, confirming the fix | Manual verification, pre-formal-test |
| 2026-09-24 | `ctest --preset windows-x64-debug` (post `c5f87c3` Format multi-section pictures) | Pass (97/97; expanded unit coverage, one new CLI test) | Local x64 CTest run |
| 2026-09-24 | `wfc --eval`, manually verifying `"(0.00)"`/`"$#,##0;($#,##0)"`-style negative pictures against both signs and zero before and after the two `REQ-0218` bug fixes | First run reproduced the wrong output for both bugs; second run (post-fix) matched the expected parenthesized/grouped rendering | Manual verification, pre-formal-test |
| 2026-09-24 | `ctest --preset windows-x64-debug` (post `e278494` Format `\` escape character) | Pass (98/98; expanded unit coverage, one new CLI test) | Local x64 CTest run |
| 2026-09-24 | `wfc --eval`, manually verifying `\0`/`\,`/`\;`/`\.` escape shapes; the self-referential `\\` case verified precisely via a C++ unit test instead, after ad hoc shell/argv backslash quoting made a command-line repro unreliable | All escape shapes matched the expected literal rendering | Manual verification, pre-formal-test |
| 2026-09-24 | `ctest --preset windows-x64-debug` (post `25e6c63` Format quoted literal text) | Pass (99/99; expanded unit coverage, one new CLI test) | Local x64 CTest run |
| 2026-09-24 | `wfc --eval`, manually verifying quoted text before/after a placeholder, a quoted `;` not splitting a picture, and quoted text combined with a real section separator | All matched the expected literal/section-selection rendering | Manual verification, pre-formal-test |
| 2026-09-24 | `ctest --preset windows-x64-debug` (post `66a5114` Format `%` scaling) | Pass (100/100; expanded unit coverage, one new CLI test) | Local x64 CTest run |
| 2026-09-24 | `wfc --eval`, manually verifying `%` scaling for a positive/negative value, escaped `\%`/quoted `"%"` suppressing scaling, and per-section scaling in a two-section picture | All matched the expected scaled/literal rendering | Manual verification, pre-formal-test |
| 2026-09-28 | `ctest --preset windows-x64-debug` (post `b04f630` IsMissing) | Pass (101/101; expanded unit coverage, one new CLI test, two stale IsMissing tests updated to match corrected behavior) | Local x64 CTest run |
| 2026-09-28 | `wfc --eval`, manually verifying an omitted/supplied `Optional Variant`, an omitted non-`Variant` `Optional` (stays False), and an omitted *defaulted* `Variant` `Optional` (stays False) | All matched real VB6's documented behavior for each case | Manual verification, pre-formal-test |
| 2026-09-28 | `ctest --preset windows-x64-debug` (post unconditional Do...Loop code change, before the stale test fix) | Hung indefinitely -- `Do\nPrint "no"\nLoop` (asserting old `WFC0040` behavior) is now a genuine infinite loop with no `Exit Do` | Local x64 CTest run; killed via `taskkill //F //T` |
| 2026-09-28 | `ctest --preset windows-x64-debug` (post `f3630b6` unconditional Do...Loop, stale test fixed) | Pass (102/102; expanded unit coverage, one new CLI test, one stale WFC0040 test corrected), run under a `timeout` guard | Local x64 CTest run |
| 2026-09-28 | `wfc --eval`, manually verifying `Option Base 1` for a 1-D array, a multi-dim array (every dimension), `ReDim`'s own shorthand, an explicit-bound dimension staying unaffected, a `ParamArray` staying 0-based, the no-`Option Base` default, and the `WFC0152`/`WFC0153` diagnostics | All matched real VB6's documented `Option Base` behavior | Manual verification, pre-formal-test |
| 2026-09-28 | `ctest --preset windows-x64-debug` (post `027d3cc` Option Base), run under a `timeout` guard | Pass (103/103; expanded unit coverage, one new CLI test) | Local x64 CTest run |
| 2026-09-28 | `wfc --eval`, manually verifying inferred `Long`/`String`/`Boolean`/`Double` constants, a derived inferred constant, and `WFC0064` still applying | All matched real VB6's Const-infers-a-concrete-type behavior (isolating one pre-existing, unrelated `&`-with-Boolean limitation hit along the way) | Manual verification, pre-formal-test |
| 2026-09-28 | `ctest --preset windows-x64-debug` (post `40cdcbe` inferred-type constants), run under a `timeout` guard | Pass (104/104; expanded unit coverage, one new CLI test) | Local x64 CTest run |
| 2026-09-28 | `wfc --eval`, manually verifying a class-typed parameter, a generic Object parameter across two classes, a `WFC0137` class mismatch, `Property Set` with a specific class-typed parameter, ByRef `Set`-reassignment write-back with correct `Class_Terminate` timing (both the bug case and the unchanged-parameter non-bug case), and `Optional Object` binding Nothing | All matched expected behavior; one pre-existing, unrelated `Else`-branch-dry-run bug found and flagged separately via `spawn_task` | Manual verification, pre-formal-test |
| 2026-09-28 | `ctest --preset windows-x64-debug` (post `1b9628c` class-typed/Object parameters), run under a `timeout` guard | Pass (105/105; expanded unit coverage, one new CLI test) | Local x64 CTest run |
| 2026-09-28 | `wfc --eval`, manually verifying a field read, a Property Let write, a Property Set write, and a method call with arguments, each in the untaken Else branch of `obj Is Nothing` | All four now correctly skip the dead branch instead of raising WFC0106 | Manual verification, pre-formal-test |
| 2026-09-28 | `ctest --preset windows-x64-debug` (post `4e4dc67` Nothing dead-branch fix), run under a `timeout` guard | Pass (106/106; expanded unit coverage, one new CLI test) | Local x64 CTest run |
| 2026-09-30 | `wfc --eval`, manually reproducing `Next j, i` at the very end of a program vs. followed by a real statement | First case appeared to work (masked by `consume_statement_end()`'s own `at_end()` tolerance); second case failed with `WFC0004`, confirming the shared-terminator structural conflict | Manual verification; led to reverting the attempt rather than a formal test |
| 2026-09-30 | `ctest --preset windows-x64-debug` (post `git checkout --` revert of the abandoned attempt, confirming a clean return to the last known-good state), run under a `timeout` guard | Pass (106/106; no change, as expected for a pure revert) | Local x64 CTest run |
| 2026-09-30 | `wfc --eval`, manually verifying a `Static Variant` reassigned via `Set` across two calls (correct termination ordering: first instance survives its own call, terminates only when the second call's copy-back replaces it) | Matched the intended fix exactly | Manual verification, pre-formal-test |
| 2026-09-30 | `ctest --preset windows-x64-debug` (post `60ae221` Static Variant object-lifetime fixes), run under a `timeout` guard | Pass (107/107; expanded unit coverage, one new CLI test) | Local x64 CTest run |
| 2026-09-30 | `wfc --eval`, manually verifying a `Static` class-typed local persisting cumulatively across three calls, and `Set` class mismatch reporting `WFC0137` | Both matched expected behavior | Manual verification, pre-formal-test |
| 2026-09-30 | `ctest --preset windows-x64-debug` (post `parse_fixed_array_bounds` extraction, before adding `Static`'s own array support), run under a `timeout` guard | Pass (107/107; no test changes, confirming the refactor alone was behavior-preserving) | Local x64 CTest run |
| 2026-09-30 | `wfc --eval`, manually verifying a 1-D `Static` array's elements persisting across three calls, a multi-dimensional `Static` array, `Static arr()` reporting `WFC0149`, and a `Variant`-element `Static` array reporting `WFC0149` | All matched expected behavior | Manual verification, pre-formal-test |
| 2026-09-30 | `ctest --preset windows-x64-debug` (post `45365b1` Static arrays and class-typed/Object locals), run under a `timeout` guard | Pass (108/108; expanded unit coverage, one new CLI test) | Local x64 CTest run |
| 2026-09-30 | `wfc --eval`, manually verifying polymorphic dispatch across two implementing classes through one `IShape`-typed variable, interface-typed parameter dispatch, `Set` rejecting a non-implementing class (`WFC0137`), and `Property Get` dispatch through a second interface (`IArea`) on a class implementing two interfaces | All matched expected behavior | Manual verification, pre-formal-test |
| 2026-09-30 | `wfc --eval`, manually verifying a `Private` interface-implementation method (`IShape_Draw`) reachable through a genuinely interface-typed reference, direct access to the same literal name still failing (`WFC0142`), and a non-interface reference's bare unprefixed name still failing (`WFC0135`) | First case initially failed with `WFC0142` (the bug this increment fixed); all three matched expected behavior after the `bypass_for_interface_dispatch` fix | Manual verification, pre-formal-test |
| 2026-09-30 | `ctest --preset windows-x64-debug` (post `c4498ac` Implements interfaces), run under a `timeout` guard | Pass (109/109; expanded unit coverage, one new CLI test) | Local x64 CTest run |
| 2026-09-30 | `wfc --eval`, manually verifying a single-level `Class_Terminate` field cascade both with and without the outer class declaring its own `Class_Terminate`, and a field's instance correctly not terminating early while a second local-variable reference to it survives | All matched expected behavior | Manual verification, pre-formal-test |
| 2026-09-30 | `wfc --eval`, manually verifying a two-level cascade (`Top` -> `Middle` -> `Deepest`) terminates in the correct outer-to-inner order | Matched expected behavior; also surfaced a separate, unrelated pre-existing gap (`o.i.tag = 9` chained field write reporting `WFC0108`), flagged via `spawn_task` rather than fixed here | Manual verification, pre-formal-test |
| 2026-09-30 | `ctest --preset windows-x64-debug` (post `9ef2f61` Class_Terminate field-cascading), run under a `timeout` guard | Pass (110/110; expanded unit coverage, one new CLI test) | Local x64 CTest run |

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
| Place `Single` below `Decimal` in the `NumericCategory` promotion order as a reasoned-but-unverified choice (superseded by increment #91 below) | The local computer-use screenshot tool became unavailable mid-session, blocking a live VB6 probe of the `Decimal`-vs-`Single` promotion order specifically; `Decimal`-above-`Currency` and `Double`-above-everything remain independently justified without a live probe | Flagged explicitly in the `NumericCategory` code comment and in `REQ-0198`'s Scope, rather than silently asserting an unverified promotion rule | `REQ-0198` |
| Reorder `NumericCategory` so `decimal_precision` sorts above `double_precision` (`Decimal` dominates `Double`, reversing the prior assumption) | A live VB6 6.00.8176 probe (see Reference Probe Evidence above) found `Decimal` dominates `Double` at both small and large magnitude, plus `Currency` and `Long`, contradicting the prior "Double dominates everything" comment, which was itself never independently verified | Corrects a real, previously-shipped behavior gap (`CDec(1) + 1234567.89` would have wrongly returned `Double` before this fix); the existing `to_decimal` widening helper needed no changes, since it already converted every lower category generically | `REQ-0198` |
| Scope dynamic arrays to "ReDim/Preserve only," deferring multi-dimensional arrays, `Erase`, `For Each`, array-typed parameters, and Variant/Object-element arrays | Owner pasted two "Remaining next increments" bullets covering all of the above at once; `AskUserQuestion` offered four escalating scope options (ReDim/Preserve alone, +multi-dim, +the rest of the first bullet, or both bullets in full) and the owner chose the smallest | Delivers a complete, working `ReDim`/`ReDim Preserve` foundation now rather than several simultaneous partial features; the other five items stay explicitly listed as future work | `REQ-0207` |
| Require `ReDim`'s target to already be `Dim`-declared as a dynamic array (`Dim identifier()`), rather than letting `ReDim` serve as an implicit first declaration the way real VB6 permits at procedure scope | A `ReDim`-as-declaration would need its own name-registration path distinct from `Dim`'s (current scope, duplicate-name checking, `As Type` parsing) for a form real VB6 itself restricts to procedure-local arrays (not module-level); narrower and simpler to require the existing `Dim identifier()` first, matching how every other WFC statement that operates on a variable already requires it declared | A disclosed simplification recorded in `REQ-0207`'s Scope, not a silent gap; `ReDim` without a prior matching `Dim identifier()` reports the new `WFC0145` | `REQ-0207` |
| Store `ArrayValue`'s element type as a `std::size_t element_type_index` (a `Value::index()`) rather than a `Value element_default` member | `Value` is `std::variant<..., ArrayValue, ...>`; a `Value` stored directly inside `ArrayValue` (not through a container's heap indirection, unlike the existing `std::vector<Value> elements`) would make `ArrayValue`'s own size depend on `Value`'s size, which depends on `ArrayValue`'s size -- an unresolvable recursive-complete-type requirement, not merely a style choice | A new free function, `array_element_default(type_index)`, reconstructs the zero value for one of the seven array-eligible scalar types on demand; verified by building immediately after the structural change, mirroring how `REQ-0201`'s original `ArrayValue{vector<Value>, Integer}` was verified before adding array behavior on top of it | `REQ-0207` |
| Give `For Each` its own statement function rather than folding it into `parse_for_statement` as an early branch | The two forms diverge immediately (a control variable requiring an exact `Long` type and a `To`/`Step` numeric range, versus any declared variable and an array expression) and share almost nothing except the trailing loop-body/`Next`-matching logic, which already lived in its own reusable `parse_for_body` function; forking early kept both functions linear and readable instead of interleaving two unrelated parameter shapes | `parse_for_body` needed no changes at all -- both callers already only depend on it via the control-variable name (for `Next` matching) and a mutable `continuation_offset`, exactly the interface it already had | `REQ-0209` |
| Make `For Each` iterate a value-copy of the array taken once at loop entry, rather than a live reference to the variable's `ArrayValue` | A live reference would need a defined answer for what happens if the loop body `ReDim`s the same array mid-iteration (resize while iterating); a snapshot sidesteps the question entirely and cannot dangle even if the array shrinks or reallocates during the loop | Simpler and strictly safer than aliasing, at the cost of not reflecting a same-loop `ReDim` in the remaining iterations -- a disclosed simplification (real VB6 actually disallows `ReDim`ing an array under active `For Each` iteration, so this rarely matters in practice) | `REQ-0209` |
| Scope multi-dimensional arrays to "fixed-size only," leaving `ReDim`/`ReDim Preserve` rejecting any comma exactly as before | A dynamic multi-dimensional array needs to track "has this array's dimension count been fixed yet" separately from ordinary allocation state (real VB6 lets a bound-less `Dim arr()` become any dimension count on its *first* `ReDim` only), a meaningfully larger feature than generalizing the existing fixed-declaration and indexing code paths | Delivers real, tested multi-dimensional indexing and declaration now rather than a partial dynamic-multi-dim model; `REQ-0207`'s `WFC0115` rejection needed no changes at all, since a multi-dim array's shape is fixed for its lifetime either way | `REQ-0210` |
| Store multi-dimensional bounds as an optional `std::vector<std::pair<Integer, Integer>> dimensions` on `ArrayValue` (empty for 1-D), rather than replacing the existing scalar `lower_bound` with a uniform dimension list | An ordinary 1-D array is the overwhelmingly common case and every existing call site (`LBound`/`UBound`, `Erase`, `For Each`, whole-array assignment, the `!execute_` dry-run placeholder) already reads `lower_bound`/`elements.size()` directly; replacing it with a always-populated list would force every one of those sites to branch on dimension count even though only indexing and `LBound`/`UBound` actually need to | Only two call sites (`parse_array_index`/`parse_array_element_assignment`'s shared index-parsing, and `LBound`/`UBound`) needed multi-dimensional awareness at all; `Erase` and `For Each` needed zero changes, confirmed by testing both directly against a 2-D array | `REQ-0210` |
| Lay out a multi-dimensional array's `elements` in row-major order (the last dimension varies fastest) | Matches real VB6's `For Each` iteration order over a multi-dimensional array, and `REQ-0209`'s `For Each` already iterates `elements` in plain flat (index 0, 1, 2, ...) order with no dimension awareness at all -- row-major is the layout that makes that existing, unmodified iteration order correct | Verified directly: summing a 2-D array's elements via `For Each` gives the same total as summing via nested indexed reads | `REQ-0210` |
| Reuse `ProcedureParameter.type_index` to hold an array parameter's required *element* type, rather than adding a separate field | Only one of "scalar type" and "array element type" is ever meaningful for a given parameter (`is_array_parameter` distinguishes which), and every other place `type_index` is already read (zero-value synthesis for an omitted Optional, arity/type-mismatch messages) either does not apply to an array parameter (Optional is rejected for one) or needs the same value an added field would have held anyway | No new field, no new branch in the handful of other `type_index` readers -- `is_array_parameter` alone is enough for `invoke_definition`'s binding loop to interpret it correctly | `REQ-0211` |
| Require an array argument to be a bare identifier (reject anything else with new `WFC0149`), rather than accepting any array-valued expression | This evaluator has no array-valued expression other than a bare variable reference anyway (no array literals, no array-returning functions), so the restriction costs nothing in practice; explicitly rejecting instead of silently mishandling a hypothetical future array expression documents the real constraint (an array parameter's write-back needs a live variable to write back *into*) | The `WFC0149` branch is unreachable today (an `ArrayValue` can only ever reach a call argument via a bare identifier, which always sets `byref_target`), kept as defensive, self-documenting code rather than an unstated assumption | `REQ-0211` |
| Check `ArrayValue.is_variant_element`/`is_object_element` directly at each element read/write site, rather than adding a `Variant`/`Object`-element array's own *name* to the existing `variant_variables`/`object_variables` scope-level sets | Those sets already govern a whole scalar/object *variable's* own rules (freely retyping on assignment, or requiring `Set`); adding an array's name to them would have made a whole-array assignment like `arr1 = arr2` (an ordinary same-type value copy, `REQ-0201`'s existing simplification) incorrectly hit the Variant-retyping or Set-only branch instead, potentially letting `arr1` retype away from being an array at all | Verified directly: whole-array assignment between two `Variant`-element arrays still works as a plain array copy, unaffected by the new per-element flags | `REQ-0212` |
| Do not extend `Class_Terminate`'s lifetime-tracking (`terminate_if_last_reference`) to an `Object`-element array's element assignment | A scalar `Object`/`Variant` variable's assignment path already calls it before overwriting; doing the same for an array element would need the same care `REQ-0204` itself documented as already incomplete (no cascading through a field) extended to array elements too -- a meaningfully separate correctness effort, not a small addition | A disclosed, recorded gap in `REQ-0212`'s Scope rather than a partially-correct attempt: an instance reachable only through an array element is not proactively terminated when that element is overwritten, `Erase`d, or the array goes out of scope | `REQ-0212` |
| Scope parenthesis-free calls to zero arguments only, deferring `Name arg1, arg2` (with arguments) and a bare `Name` statement with no `Call` keyword at all | `AskUserQuestion` offered this as the narrowest, lowest-risk slice of the deferred parenthesis-free-call gap several requirements had flagged; a with-arguments form reintroduces the classic ambiguity between a parenthesized single-argument call and other statement/expression shapes (e.g. `Name (1)` vs `Name(1)`), while zero arguments has no such ambiguity -- there is nothing to parse between the name and the statement/expression boundary | Delivers the concretely-requested case (`Print Rnd`) and every other niladic call for free, without touching the higher-risk argument-list disambiguation problem at all | `REQ-0213` |
| Detect "not a recognized intrinsic function name" by speculatively calling `parse_function_call` and checking for its own `WFC0071` failure with no input consumed, rather than duplicating its ~40-flag `is_xxx` name list in a separate lookup | The alternative -- a second, independently-maintained list of every intrinsic function name -- would drift out of sync with the real dispatcher's list over time; `parse_function_call`'s own not-recognized check already runs before any argument parsing (before `consume('(')`), so a `WFC0071` failure with `offset_` unchanged is an exact, side-effect-free signal that no call was ever actually attempted | Verified with a genuinely undeclared name (`WFC0015`, not `WFC0071`) and a recognized-but-wrong-arity name (`Len`, correctly `WFC0072`, not silently swallowed) | `REQ-0213` |
| Store a class-typed array's declared class name directly on the `ArrayValue` (new `element_class_name` field), rather than in the existing scope-level `object_class_names` map | `object_class_names` is keyed by variable name for a whole scalar/object variable's own `Set`-target class check; the same "array's own name doesn't belong in a per-variable scalar-rules map" reasoning `REQ-0212` already established for `variant_variables`/`object_variables` applies here too | `assign_object_reference` needed no signature or logic change at all -- only the `declared_class_name` argument its two existing callers pass now reads from the array's own field instead of always passing an empty string | `REQ-0214` |
| Give the array return-value slot an unallocated dynamic array as its starting value, rather than a fixed-size array sized from some inferred default | A `Function`'s array return type declares only an element type, never a size (`Function name() As Type()`, no bound expression) -- exactly the shape `REQ-0207`'s existing dynamic-array machinery already represents, letting the body build the result via a plain array assignment or a direct `ReDim` with zero new array-side logic | Verified both forms independently: assigning a separately-built local array to the return name, and `ReDim`ing the return name directly inside the body | `REQ-0216` |
| Apply the new array-return-type marker (`As Type()`) only at the two `Function` return-type parsing sites, not at `Property Get`'s or a class field's shared `parse_scalar_object_or_class_type` call sites | `Property Get` returning an array and an array-typed class field are each their own, separately-scoped feature (neither requested); wiring the check in at the two `Function`-specific call sites (immediately after each one's own `parse_scalar_object_or_class_type` call) keeps every other caller's behavior completely unchanged, with nothing to accidentally enable | `REQ-0216`'s Scope records both as explicit future work rather than silently-untested gaps | `REQ-0216` |
| Render `Format`'s `Currency` style with a fixed `$` prefix and a leading `-` for negative values, rather than deferring the style further until a locale system exists | This evaluator has no locale system at all and none is planned soon; real VB6's own `Currency` style is itself locale-dependent (system currency symbol, commonly a parenthesized negative convention), so no rendering could honestly claim to match every locale anyway -- a fixed, disclosed convention is more useful than continuing to defer the whole style | Recorded explicitly as a reasoned-but-unverified choice in `REQ-0193`'s Scope, matching this project's established pattern for locale-shaped gaps (e.g. `Rnd`'s WFC-owned seed hash) rather than silently asserting a false match | `REQ-0193` |
| Fix the fully-bare `obj.Method` statement (no `Call` at all) with a lookahead inside `parse_assignment_or_array_element`'s existing object-dot branch, rather than folding `parse_member_assignment` and `parse_member_access_after_dot` into one function | The two functions serve genuinely different jobs (assignment vs. expression-read) with only partial overlap; merging them would touch every existing caller of both for a narrow gain, where a lookahead -- parse the member name, check it is a method with nothing following, decide which function to hand off to -- is a small, local, easily-verified addition | Verified `obj.Prop = expr` and the indexed `obj.Prop(i) = expr` (`Property Let`) both still resolve through the unchanged `parse_member_assignment` path exactly as before | `REQ-0217` |
| Treat *any* `Format` `Style` that does not name a reserved named style as a custom numeric picture, rather than reporting `WFC0102` for a string that looks malformed | Real VB6 itself never rejects a `Style` string outright -- every character is either a recognized token or a literal, so there is no "invalid picture" to detect; matching that meant `WFC0102` (previously reported unconditionally in this exact fallback) became genuinely unreachable, not merely rare | The one existing `WFC0102`-under-`Format` test (`Format(42, "Nope")`) was removed rather than updated, since `"Nope"` is now valid input (an all-literal picture rendering `"42Nope"`) | `REQ-0218` |
| Reuse `std::to_chars(..., chars_format::fixed, N)` for the picture's fraction rounding, generalizing the fixed literal `2` `render_fixed_style` already hardcodes to the picture's own placeholder count `N` | `Fixed`/`Standard`/`Percent` already establish this exact rounding behavior via the same `to_chars` call; reusing the identical mechanism (just parameterized) guarantees a custom picture's rounding is never subtly different from the named styles' own, with no new rounding logic to separately verify | Verified `Format(1234567.891, "#,##0.00")` matches `Standard`'s own rounding of the same value exactly | `REQ-0218` |
| Apply the "a placeholder sets a minimum width, never a maximum" rule literally even when a picture's integer section has *no* digit placeholders at all (e.g. `"hello"`) | A dedicated special case ("no placeholders = discard the value entirely") would be an arbitrary carve-out with no stronger textual-fidelity justification than the general rule already gives; applying the same rule uniformly needed no extra code at all -- the existing right-to-left match loop and its "push any leftover digits" tail already produce this result unmodified | Disclosed in `REQ-0218`'s Scope as a reasoned interpretation, not independently verified against the reference runtime, since this exact input shape is unlikely to occur in real-world custom pictures | `REQ-0218` |
| Give a dynamic array's dimension count two ways to become fixed -- comma pre-declaration (`Dim arr(,)`) or the array's own first `ReDim` -- rather than requiring pre-declaration always | Real VB6 supports both `Dim arr()` (dimension count from the first `ReDim`) and `Dim arr(,...)` (dimension count fixed up front); requiring only one would reject valid VB6 source | Verified both forms independently: a plain `Dim arr()` accepts any first `ReDim`'s shape then locks it, while `Dim arr(,)` rejects a mismatched first `ReDim` immediately | `REQ-0219` |
| Store the new `dynamic_dimension_count` field at the very end of `ArrayValue`, not next to the conceptually-related `dimensions` field | Every existing call site constructs `ArrayValue` via positional aggregate initialization; inserting a field in the middle would silently misassign every later positional argument at every one of those call sites, a class of bug that is easy to introduce and hard to notice | Set via post-construction assignment only in the one branch that needs a non-default value, leaving every other call site's positional argument list untouched | `REQ-0219` |
| Generalize the existing 1-D `ReDim`/`ReDim Preserve` overlap-copy algorithm to N dimensions instead of writing a separate multi-dim code path | The 1-D case is exactly N=1 of "hold every outer dimension's bounds fixed, overlap-copy the last" -- one algorithm covers both without special-casing, and any future correctness fix to the copy logic automatically applies to both | Verified the existing 1-D `ReDim`/`ReDim Preserve` tests still pass unchanged through the generalized code path, alongside new 2-D-specific tests | `REQ-0219` |
| Fix the `Erase`-on-multi-dim-dynamic-array bug by clearing `dimensions` and extracting a shared `array_expected_dimension_count()` helper, applied at all four call sites that need an array's dimension count, rather than a narrow one-line fix in `Erase` alone | The other three call sites (`Set arr(i) = expr`, array-element write, array-element read) each independently computed the same value with the same latent flaw (always-1 for an unallocated pre-declared array); fixing only `Erase` would have left that flaw live and undiscovered until the next thing tripped over it | Re-ran the originally-hanging manual repro under a `timeout` guard to confirm it now completes and reports `WFC0111`; full local suite stayed at 100% after the fix | `REQ-0219` |
| Give a multi-section picture's negative section the value's magnitude rather than a separate "suppress the automatic sign" flag threaded through the single-section renderer | Passing a non-negative number means the existing single-section helper's own `negative = value < 0.0` check is simply false already -- reusing that one existing code path exactly, unmodified, is strictly simpler than adding a parameter every call site (including the single-section one) would need to thread through | Verified the negative section's own literal characters (parens, a trailing `-`) are the only source of any sign in a multi-section negative rendering, with no automatic `-` ever added on top | `REQ-0220` |
| Fix the two `REQ-0218` overflow-digit/comma-grouping bugs `REQ-0220`'s own manual testing found, rather than disclosing them as further unverified edge cases the way `REQ-0218`'s original Scope did for a related case | Both bugs produce outright *wrong* output (not merely unverified output) for the single most common real-world use of a multi-section picture -- a parenthesized or currency-prefixed negative section -- so shipping `REQ-0220` without fixing them would make its own headline use case visibly broken | Added regression tests for both exact failure shapes (`"(0)"` with more digits than placeholders, `"$#,##0;($#,##0)"` negative with grouping) directly in the same increment that found them | `REQ-0220` |
| Expand every `\`-escaped pair into a parallel `(text, forced_literal)` representation up front, rather than teaching each existing check (digit placeholder, decimal point, grouping comma, section separator) to look one character back for an escaping `\` itself | A single up-front expansion pass keeps every downstream check a simple by-index `!forced_literal[i]` test, with no risk of one check's own "was the previous character a `\`" logic drifting out of sync with another's -- especially important once `REQ-0220`'s own `;` section-splitting also had to become escape-aware in the same change | Verified every escape shape (`\0`, `\,`, `\.`, `\;`, `\\`, and `\;` combined with a real, later, unescaped section separator) renders correctly with this one shared mechanism | `REQ-0221` |
| Give `"..."` quoted literal text no special interaction with `\` at all (a `\` inside a quoted run is just an ordinary literal character, not an escape) | Real VB6's own documentation does not describe the two mechanisms interacting, and the two features already serve the identical purpose (forcing a literal); inventing an interaction rule with no reference to verify it against would be a guess dressed up as a decision | Disclosed as an unverified simplification in `REQ-0222`'s Scope, rather than silently picked without a note | `REQ-0222` |
| Detect `%` scaling as a simple presence flag on the whole section (scale by 100 exactly once if any unescaped `%` appears anywhere in it), rather than tracking how many `%` characters appear or where | Real VB6 treats `%` as a per-section flag, not a per-occurrence multiplier -- a picture with two `%` characters still only scales by 100 once, matching the named `Percent` style's own single, fixed scaling factor | Verified a picture combining `%` with other characters (`"0%"`, `"0.00%"`) scales exactly once regardless of digit-placeholder count or picture length | `REQ-0223` |
| Give `IsMissing` its own bare-identifier argument grammar, parsed before the shared generic `parse_expression()`-per-argument loop every other intrinsic function uses, rather than trying to recover the parameter's name from an already-evaluated argument `Value` | An omitted Optional argument is synthesized into a real, ordinary `Value` (the default) before the generic loop would ever see it -- indistinguishable from a caller-supplied value of the same type -- so there is no way to answer correctly without the argument's own name, which only a dedicated identifier-level parse (instead of expression evaluation) can preserve | Verified `IsMissing(x)` still resolves correctly inside a `Function`/`Sub` body alongside every other unrelated intrinsic call on the same line, confirming the special-cased branch does not disturb the shared dispatch's normal argument handling for any other function | `REQ-0224` |
| Exclude a *defaulted* `Optional Variant` parameter (`Optional x As Variant = 5`) from `IsMissing`'s `True` case, even though it is still both Optional and Variant | `REQ-0206`'s own Scope section, written when `IsMissing` was first deferred, already documented this exact real-VB6 nuance (the default counts as supplied); implementing `IsMissing` without checking `has_default` too would have silently contradicted that requirement's own prior research | Added a dedicated regression test for the defaulted case (`Optional x As Variant = 5`), both omitted and supplied, confirming both answer `False` | `REQ-0224` |
| Dispatch an `Explore` subagent to re-scan every `requirements/language/*.md` Scope section for a small deferred feature, rather than picking the next item by memory or re-reading the whole folder inline | The work log's own aggregated "Remaining next increments" list had been fully worked down to items that are each individually larger or riskier than the increments just completed (class inheritance, `CVErr`, `ReDim`-as-declaration, `E+`/`E-` scientific notation); a systematic scan across every requirement doc's own Scope section, not just the aggregated summary, was needed to find something still genuinely small before escalating to a bigger item | The subagent's report was independently spot-checked against `src/evaluator.cpp` before acting on it (confirmed `parse_posttest_do_statement`'s exact `WFC0040` branch and line numbers) rather than trusted blind | Process |
| Fix the hung test suite by updating the one stale `WFC0040` test to use genuinely malformed syntax, rather than reverting or special-casing the unconditional-loop feature to avoid the collision | The old test's own input (`Do\nPrint "no"\nLoop`) was asserting behavior this requirement deliberately and correctly changes -- it is now valid, intentionally non-terminating-without-`Exit Do` syntax, the same situation every prior "old test asserted now-superseded behavior" fix in this log has handled the same way | Re-ran the full suite under a `timeout` guard after the fix to confirm 100% pass with no further hangs | `REQ-0225` |
| Give `Option Base` its own single evaluator-wide `option_base_one_` flag rather than any per-module or per-class scoping | `Option Explicit`/`Option Compare` already made this exact simplification (this evaluator has only one standard module plus a flat `--class` list, with no independent per-class `Option` scope at all); matching that existing precedent kept the three `Option` directives consistent with each other rather than introducing scoping for only the newest one | Verified `Option Base`, like the other two, must appear before any declaration and is rejected a second time in the same module, exactly mirroring `Option Explicit`/`Option Compare`'s own tested placement rules | `REQ-0226` |
| Leave a `ParamArray`'s own array always `0`-based regardless of `Option Base`, rather than applying the setting there too for consistency | This is real, documented VB6 behavior, not a simplification -- `Option Base` never reaches a `ParamArray`'s implicit array at all in the reference language; applying it there anyway would be a fabricated rule, not a faithful implementation | Verified directly by manual test (`ParamArray` under `Option Base 1` still reports `LBound = 0`) rather than left as an assumption, since this was flagged as needing confirmation rather than being self-evident | `REQ-0226` |
| Give an inferred-type `Const` whatever concrete type its initializer naturally evaluates to (this evaluator's own existing literal-typing rules), rather than matching real VB6's own "smallest fitting type" inference or defaulting to `Variant` (mirroring a bare `Dim`'s own rule) | `Const`'s real-VB6 default is a concrete type, not `Variant` -- `Dim`'s Variant-default and `Const`'s type-inference are a genuine, documented VB6 asymmetry, not two implementations of the same idea; and reusing this evaluator's pre-existing, already-disclosed literal-typing convention (unsuffixed integers are `Long`, `REQ-0199`'s Scope) avoids inventing a second, Const-specific inference rule that would only ever apply in this one place | Verified `Const x = 5` reports `TypeName(x) = "Long"` (this evaluator's existing convention, not real VB6's `Integer`), disclosed explicitly in `REQ-0227`'s own Scope rather than left implicit | `REQ-0227` |
| Dispatch a second `Explore` subagent specifically to re-scope the class-features backlog cluster into concrete sub-items, rather than attempting one of its bigger-sounding bullets (class inheritance, COM interop) directly or picking a sub-item from memory | The cluster's own backlog phrasing bundled several genuinely large items with at least one that turned out small (class-typed parameters); a fresh, targeted investigation of exactly which named sub-items were actually small was cheaper and safer than guessing, matching the same successful pattern the first subagent dispatch (unconditional Do-Loop/Option Base/inferred-type Const) already established | The subagent's own report flagged the ByRef write-back lifetime risk *before* any code was written, which the implementation then confirmed was real and fixed -- the investigation step paid for itself | Process |
| Fix the `Class_Terminate`-skipped-at-ByRef-write-back bug generally (call `terminate_if_last_reference` on every ByRef write-back target, not just object-typed ones), rather than special-casing it only for `is_object_reference` parameters | `terminate_if_last_reference` is already a documented no-op for any non-`ObjectInstance` value; a single unconditional call at the one write-back site is simpler than threading a parameter-kind check through it, and automatically covers any future parameter kind that might someday also hold an object reference | Verified the ordinary unchanged-object-parameter case is not prematurely terminated (the callee's own frame still holds a second reference at the moment of the check, so `use_count() != 1` correctly skips termination) alongside the actual bug case | `REQ-0228` |
| Flag the pre-existing `If obj Is Nothing Then ... Else obj.Member End If` dry-run bug via `spawn_task` for separate follow-up, rather than fixing it inline as part of this increment | The bug is real but entirely unrelated to class-typed parameters (reproduces with a plain `Dim o As Object`, no parameters at all) -- fixing it would have widened this increment's diff into unrelated dry-run/If-statement territory instead of keeping the parameter feature's own diff reviewable on its own | Confirmed via manual repro with `Dim o As Object` (isolating it from parameters entirely) before flagging, so the follow-up task states a verified, reproducible bug rather than a hunch | `REQ-0228` |
| Dismiss the just-created `spawn_task` for the Nothing-dead-branch bug and fix it directly in this same session instead, rather than leaving it for a separately-dispatched session to pick up | This session already had the exact repro, root cause, and all four call sites loaded in context; a fresh session would have to re-derive all of that from scratch, making the flag-and-defer path strictly more expensive once the fix was already well-understood | Verified the fix, wrote its own requirement doc, and committed it separately from `REQ-0228`'s own commit (matching the two-part pattern this log already uses for "found and fixed a bug" narratives) | `REQ-0229` |
| Leave `CStr(Nothing)`/`Nothing` concatenation's own `WFC0106` unconditional (not suppressed in a dead branch), while fixing the *member-access* `WFC0106` sites to suppress it | The two are different in kind: member access on `Nothing` fails only because of runtime *state* (the reference happens to be unset), matching arithmetic overflow's own "value-dependent" category this evaluator already suppresses in dead code; `CStr(anyObjectReference)`/`Nothing &` are *always* invalid regardless of live state, closer to the pre-existing `WFC0020` "wrong operand type" category, which already fires in dead branches too | Deliberately left both sites unedited and said so explicitly in `REQ-0229`'s own Scope, rather than changing behavior with no test covering the decision | `REQ-0229` |
| Revert the comma-separated `Next` attempt entirely, rather than shipping a partial version scoped to "exactly two loops, `WFC0154` beyond that" | The two-loop restriction only avoided the *comma-count* problem (a third name in one list); it did nothing about the deeper shared-terminator problem, which breaks the *two*-loop case too as soon as a real statement follows the `Next` line -- there was no smaller, honest subset left to ship once that was found, only a broken one or a fully-fixed one | Confirmed the break with a minimal repro (a real statement after `Next j, i`) before deciding to revert, rather than reverting on the first sign of trouble | `REQ-0230` (attempted, reverted) |
| Reuse the requirement number the reverted `Next`-comma-list attempt never actually consumed in any committed file, rather than skip it | The attempt's own in-progress code comments referenced "REQ-0230," but nothing referencing that number was ever committed (confirmed via `grep` across the work log and requirements folder before reusing it) -- reusing it keeps the numbering sequential instead of leaving a permanent gap for work that was never shipped under that name | Verified via `grep -rn "REQ-0230"` returning nothing prior to writing this fix's own doc | `REQ-0230` |
| Extract `parse_fixed_array_bounds` as a shared helper before adding `Static`'s own array support, rather than duplicating the bound-list grammar inline a second time | `Dim`'s own fixed-size array bound parsing (an expression, optional `To`, `Option Base`'s default lower bound, comma-separated dimensions) was already a self-contained ~50-line block with no dependency on anything `Dim`-specific once past the opening `(` -- duplicating it for `Static` would have meant two copies to keep in sync for every future array-bound fix (`Option Base`'s own addition, `REQ-0226`, already had to touch this exact logic once) | Ran the full suite with *no test changes at all* immediately after the extraction, confirming it was purely mechanical before adding any new `Static`-specific behavior on top | `REQ-0231` |
| Give `Static` arrays no dynamic (bound-less/comma-only) form at all, rather than mirroring `Dim`'s own dynamic-array/`ReDim` support | This is real VB6 behavior, not a simplification -- a `Static` array's bounds are fixed at declaration time with no `ReDim` counterpart ever possible for one, unlike a `Dim`'s array; matching `Dim`'s own dynamic form here would be adding a feature real VB6 itself does not have | `Static arr()` reports `WFC0149` directly rather than being silently accepted as (or confusingly failing as) an attempted dynamic array | `REQ-0231` |
| Implement `Implements` interfaces next (owner: "do interfaces"), ahead of class inheritance, COM interop, lazy `As New`, or `Class_Terminate` field-cascading | Direct owner instruction naming the specific backlog item, not a subagent ranking | Continues closing the class-cluster backlog `REQ-0203`'s own Scope originally listed, following `REQ-0230`/`REQ-0231`'s completion of the `Static` half of the same cluster | `REQ-0233` |
| Thread interface dispatch as a static-declaration-driven rewrite (`via_interface_class` parameter on `parse_member_access_after_dot`), populated only at the three call sites that can cheaply obtain a base expression's declared interface type, rather than adding a runtime "current static type" concept to every `Value` | This evaluator's dispatch model is purely runtime-instance-based with no existing representation of "the static type currently viewing this reference"; adding one everywhere would be a much larger, riskier change than threading one optional parameter through the single function that already does all member-access resolution | Manually verified dispatch through a `Dim`/`Static` local and through a parameter; deliberately left dispatch through a field, a function return, or an array element unthreaded (Scope), verified those still work via `Call Me.Method` semantics being unaffected | `REQ-0233` |
| Bypass the per-class `Private`-visibility check only for a call genuinely routed through the declared interface (`bypass_for_interface_dispatch`), rather than making the interface's own generated-name convention implicitly public | Real VB6's own `InterfaceName_MemberName` convention is nearly always declared `Private`, since it is not meant to be called any other way; a blanket "interface members are public" rule would have made direct, non-interface access to the same name incorrectly succeed, weakening `REQ-0206`'s existing per-class visibility guarantee for every other `Private` member | Verified both directions after the fix: dispatch through the interface succeeds, while direct access to the literal `InterfaceName_MemberName` name from outside the class still correctly reports `WFC0142` | `REQ-0233` |
| Do not add completeness verification (checking every interface member has a matching `InterfaceName_MemberName` implementation), property/field writes through an interface reference, `TypeOf ... Is`, or interface inheritance | Each is a materially separate feature from the core ask ("do interfaces"); a missing implementation is already caught, just lazily, by the pre-existing `WFC0135` the first time dispatch reaches it, matching this evaluator's general preference for reporting errors where they are hit rather than a separate static pass | Disclosed explicitly in `REQ-0233`'s own Scope rather than silently left untested | `REQ-0233` |
| Implement `Class_Terminate` field-cascading by reusing `drain_scope_instances(instance->fields)` unchanged inside `terminate_if_last_reference`, rather than writing new field-walking logic | `InstanceData::fields` is itself a `Scope`, identical in shape to a call frame's locals `drain_scope_instances` already knows how to drain correctly (including the "clear as it goes" ordering that keeps `use_count` accurate between aliasing references); reusing it means the cascade also recurses to arbitrary depth for free, since a cascaded field's own termination is just another ordinary call to `terminate_if_last_reference` | Verified a two-level cascade (`Top` -> `Middle` -> `Deepest`) terminates in the correct outer-to-inner order with no cascade-specific code beyond the one added call | `REQ-0234` |
| Cascade into a dying instance's fields regardless of whether that instance's own class declares `Class_Terminate` | An instance's fields go out of scope along with it either way -- gating the cascade on the outer instance happening to have its own `Class_Terminate` would leave the exact same gap for the (arguably more common) case of a plain container class with no lifecycle hook of its own | Verified the cascade fires correctly when the outer class (`Outer` in the test) declares no `Class_Terminate` at all | `REQ-0234` |
| Flag the discovered `o.i.tag = 9` chained-field-write `WFC0108` bug via `spawn_task` for separate follow-up, rather than fixing it inline here | A chained multi-level assignment-target parsing gap is a materially separate feature from lifetime cascading -- fixing it inline would have expanded this increment's diff into an unrelated area of the parser | Every manual repro and formal test in this increment uses an intermediate local variable at each level instead, avoiding the gap entirely rather than working around it silently | `REQ-0234` (deferred) |
| Implement the distinct 16-bit `Integer` type next (owner said "keep working on P2" — MP-0002 — confirmed via a clarifying question since "P2" was ambiguous); chosen from the work log's own "Remaining next increments" list, which named it first | Owner request, disambiguated via `AskUserQuestion` | Continues the established per-type-increment pattern (`Single`, `Currency`, `Decimal`) for the one remaining VB6 intrinsic numeric type | `REQ-0199` |
| Give `Integer` its own C++ type alias (`Int16` = `std::int16_t`) rather than reusing the existing `Integer` alias (which is actually `std::int32_t`, VB6's `Long`) | The existing `Integer` C++ alias name predates this type and already means "Long" throughout the codebase; renaming it now would touch hundreds of call sites for no behavioral benefit | A one-time naming collision between VB6's `Integer` and this codebase's pre-existing `Integer` alias, documented at both declarations, is less disruptive than a global rename | `REQ-0199` |
| Do not add `Integer`-widening to the fixed-type `Long`-target assignment path (`Dim x As Long: x = someInteger` still fails `WFC0016`) | Discovered mid-implementation: assignment to a `Long`-typed variable has never coerced from *any* other numeric type in this evaluator (confirmed for `Currency`/`Single`/`Double` sources too, via direct probing) — a pre-existing, evaluator-wide scope boundary, not specific to `Integer` | Keeps `Integer`'s behavior consistent with every other type's existing relationship to `Long`-typed assignment targets, rather than special-casing `Integer` alone | `REQ-0199` |
| Do not extend `Choose`'s index, `QBColor`'s color index, `RGB`'s components, or other strictly-`Long` intrinsic-function parameters to accept `Integer` | Confirmed these parameters were never extended to accept `Single`/`Currency`/`Decimal` either (probed `Choose(2@, ...)`, which already fails today) — an existing scope boundary from every prior numeric-type increment | Keeps this increment's scope aligned with the established "only the documented function list, not every `Long`-parameter function" precedent | `REQ-0199` |
| Scope arrays to "fixed-size 1-D only" (no `ReDim`, no multiple dimensions, no `Preserve`, no array-typed function parameters/returns, no `For Each`) and objects to a "minimal stub" (`Nothing`/`Set`/`Is`/`IsObject` only, no class modules/`New`/property-or-method access) | Owner decision (`AskUserQuestion`), given as two scoping choices after the owner's bare instruction "implement arrays and object references next" — both features individually cover a wide range of possible depth, and a full object model in particular is closer to a new language feature than a type increment | Delivers working, testable array indexing and a real `Nothing`/`IsObject`/`Is` foundation now, rather than an open-ended architecture effort; both REQ docs' Scope sections enumerate the deferred items explicitly | `REQ-0200`, `REQ-0201` |
| Represent an array as a new `ArrayValue{std::vector<Value>, Integer lower_bound}` `Value` alternative, making `Value` a genuinely recursive type (an array can be a `Value`, and a `Value` can be an array) | The alternative -- indirection via `unique_ptr`/`shared_ptr` with an explicit out-of-line destructor -- adds Rule-of-5 boilerplate for no behavioral benefit, since C++17 `std::vector<T>` already supports an incomplete `T` at class-member-declaration time, becoming valid once `T` (here, `Value`) is complete before the vector's own methods are actually used | Verified by building immediately after the structural change, before writing any array behavior on top of it: forward-declare `ArrayValue`, declare the `Value` alias naming it, then define `ArrayValue`'s body referencing `std::vector<Value>` -- compiles cleanly on MSVC's STL | `REQ-0201` |
| Give `Object`-typed variables their own `object_variables_` tracking set (mirroring `variant_variables_`), rather than inferring "is this an Object" from "does it currently hold `Nothing`" | A `Variant`-declared variable can also currently hold `Nothing` (via `Set`), and the two cases need different assignment rules: plain `=` is rejected only for a fixed `Object` target, not for a `Variant` one currently holding `Nothing` | Correctly distinguishes "this variable's fixed declared type is Object" from "this variable happens to hold Nothing right now" without adding a new `Value` state | `REQ-0200` |
| Reject `Variant`- and `Object`-element arrays (`Dim arr() As Variant`/`As Object`) rather than attempting per-element retyping or per-element `Nothing` tracking | `Variant`'s existing retyping design is tracked per *variable name* (`variant_variables_`), not per array *element*; extending that to element granularity is a meaningfully larger feature, and array-of-`Object` has nothing productive to hold without class modules anyway | Kept explicitly in each REQ's Scope rather than silently producing a wrong result for these combinations (`Dim arr() As Variant` reports the same `WFC0012` "unrecognized As-clause type" diagnostic a genuinely unsupported type reports elsewhere) | `REQ-0201` |
| Build user-defined procedures first, as their own increment, before any class-module work | Owner request ("finish objects"), redirected via `AskUserQuestion` after discovering "finish objects" actually depends on procedure scope, which did not exist at all (flagged in `REQ-0141`'s Tailoring section since MP-0002's second increment) | Delivers a real, working `Sub`/`Function`/recursion/scope foundation now, rather than starting class modules on top of nothing; a class-module layer remains a distinct, separately-scoped future increment | `REQ-0202` |
| Refactor the four flat `variables_`/`constants_`/`variant_variables_`/`object_variables_` members into a `Scope` struct plus a `scopes_` scope-chain, with lookups checking only the current frame and the module frame (never an intermediate caller's frame) | Procedure calls need real local variable scope (parameters and local `Dim`s invisible outside the call, distinct from module-level variables) and recursion needs each call to get its own independent local storage; VB6 itself has no scope deeper than module/procedure, so a full nested lexical-scope stack was not needed, only two visible levels | A ~28-call-site mechanical refactor (every existing `variables_.find`/`.contains`/`.emplace`/`.insert` call site), done before writing any new procedure-call logic on top of it, verified against the full existing test suite before proceeding | `REQ-0202` |
| Switch `scopes_` from `std::vector<Scope>` to `std::deque<Scope>` | Discovered via live testing during this same increment: a `ByRef` argument's write-back pointer (captured before a call pushes its new frame) silently stopped working, and deep recursion segfaulted -- both traced to `std::vector::push_back` reallocating on growth, which can copy (rather than move) existing elements and orphan pointers previously taken into them; `std::deque` guarantees push/pop at the ends never invalidates references or pointers to existing elements | Fixed both symptoms; verified with the same `ByRef` and recursive-Factorial cases that first exposed the bug, plus the full test suite | `REQ-0202` |
| Bound procedure-call recursion at a fixed depth (`WFC0123` beyond 64 levels) rather than leaving it unguarded | Since each VB6-level call recurses through this evaluator's own C++ call chain (`run_procedure_body` → `parse_statement` → ... → `parse_procedure_call` → `call_procedure` → `run_procedure_body` again), unbounded VB6 recursion can overflow the native call stack -- a process crash, observed directly in this session between roughly 100 and 128 levels in a local debug build | 64 was chosen empirically for a comfortable margin below the observed crash range (a release build, with smaller per-frame stack usage, has more headroom); verified a runaway self-recursive function fails cleanly with `WFC0123` instead of crashing, while ordinary recursion (`Factorial(10)`) still succeeds | `REQ-0202` |
| Supply each class module as its own separate source (`wfc::ClassModuleSource`/CLI `--class <Name> <source>`) rather than an inline `Class ClassName ... End Class` block in the same source text | Owner decision (`AskUserQuestion`): mirrors a real VB6 project's separate `.cls` files, at the cost of extending the CLI/API to accept multiple named sources instead of one flat blob | This evaluator had no multi-file/multi-module concept at all before this increment; chosen over the inline-block alternative specifically because it matches real VB6 project structure rather than inventing non-standard syntax | `REQ-0203` |
| Cover fields, methods, and `Property Get`/`Let`/`Set` accessors together in this first class-modules increment, rather than fields and methods alone | Owner decision (`AskUserQuestion`), given alongside the declaration-form and lifecycle choices | Delivers real encapsulation (computed properties, a way to expose an object reference without a class-typed field) in the same increment as basic instantiation, rather than a still-incomplete fields-only class | `REQ-0203` |
| Instantiate `Dim x As New ClassName` eagerly, at the `Dim` statement itself, rather than VB6's lazy auto-instantiation (created on `x`'s first actual use) | Owner decision (`AskUserQuestion`): "New only" was chosen over also adding `Class_Initialize`/`Class_Terminate`, and eager instantiation was the simpler of the two `As New` semantics to build correctly first | Lazy semantics would need a third variable state ("declared `As New`, not yet instantiated") tracked per read, not just per `Set`; documented as an explicit, disclosed simplification rather than attempted this increment | `REQ-0203` |
| Give class fields no `Object`/class-typed option (the same eight-scalar-type list `Dim` already accepts, minus `Object`) | Matches `REQ-0202`'s own existing precedent excluding `Object` from procedure parameter/return types; an object reference can still be stored in a `Variant` field (already possible since `REQ-0197`/`REQ-0200`) | Kept scope bounded to what a `Property Set` member actually needs (a `Variant` backing field), rather than also solving class-typed-field storage/type-checking in the same increment | `REQ-0203` |
| Resolve an unqualified call/read inside a class method against that method's *own* class's sibling members (methods and `Property Get`) before falling back to "undeclared", rather than requiring an explicit `Me.` qualifier | Discovered mid-implementation: the increment's own recursion test (`Factorial` calling itself, and a second method calling `Factorial`, both unqualified) failed with "unsupported function" until this was added -- `Me` itself is not implemented, so without this fallback a class could never call its own methods at all | Matches real VB6, where a class module's own members are directly callable unqualified; `instance_scopes_` was switched from a `Scope*` stack to an `InstanceData*` stack specifically so the current instance's *class* (needed to look up a sibling method) is recoverable, not just its field storage | `REQ-0203` |
| Leave unqualified *writes* to a sibling `Property Let`/`Set` (from within another method, with no `Me.` qualifier) unsupported this increment, even though unqualified reads/calls are | `parse_assignment`'s bare-identifier statement path has no member-dispatch hook the way `parse_primary`/`parse_call_statement` were extended to have; adding one is a smaller, separate follow-up rather than blocking this increment | A narrower, disclosed gap: external `obj.Prop = expr` and unqualified sibling method/`Property Get` access both already work; only the combination (unqualified + write + Property) is deferred | `REQ-0203` |
| Give `InstanceData` `enable_shared_from_this` so `Me` can hand out a new `ObjectInstance` | The alternative -- wrapping the raw `InstanceData*` `instance_scopes_` already tracks in a brand-new `shared_ptr` -- would create a second, independent control block; once both it and the original reached zero, the same `InstanceData` would be freed twice | `enable_shared_from_this` shares the *existing* control block instead, since `InstanceData` is only ever created via `make_shared`; verified with `Set y = x.GetSelf()` (a `Function As Variant` returning `Me`) followed by `y Is x` | `REQ-0204` |
| Do not hook `Class_Terminate` onto `~InstanceData()`'s own C++ destructor; fire it only from three explicit points instead (a `Set` overwrite, a call frame's locals as the call returns, and the module scope at the end of a successful program) | A destructor hook would fire whenever the last `shared_ptr` reference happens to be dropped, including *during* another container's own teardown -- a `Scope`'s `variables` map destroying its `Value`s as part of `scopes_.pop_back()`, or the `Interpreter`'s own final member destruction at program end -- and reentrantly calling back into the evaluator (pushing a new call frame onto a `scopes_` that is itself mid-`pop_back()`) at that point is undefined behavior, not merely awkward | Fires `Class_Terminate` only while the interpreter is fully alive and not itself mid-teardown of anything, at the documented cost of three gaps (no firing for an unbound temporary instance, no cascading to an instance only reachable through the terminated one's own fields, and same-frame aliases relying on drain order) recorded in `REQ-0204`'s Scope | `REQ-0204` |
| Drain a call frame's/the module scope's ObjectInstance-holding variables one at a time -- terminate-then-clear-to-Empty each, in sequence -- rather than checking every variable first and clearing them all afterward | Two variables in the *same* frame can alias the same instance (e.g. `Dim a As New Foo` then `Set b = a`); checking all of them before clearing any would show every alias with a use_count inflated by the others still being live, so none would ever look like "the last reference" | Verified directly: `Dim a As New Foo`/`Set b = a`/end of `Sub` fires `Class_Terminate` exactly once, at whichever of `a`/`b` the unordered_map iteration happens to drain second | `REQ-0204` |
| Only drain the module scope (and therefore only fire any surviving module-level instance's `Class_Terminate`) on `evaluate()`'s successful-completion path, not on any error return | Every other return in `evaluate()` is already reporting a fatal error; running more class-member code during that unwind seemed more likely to compound the failure (or mask the original error with a new one from inside `Class_Terminate`) than to clean up after it | A disclosed, deliberate scope boundary rather than an oversight -- recorded explicitly in `REQ-0204`'s Scope | `REQ-0204` |
| Reorder `parse_primary_base`'s unqualified-identifier resolution to try `find_variable` (local parameter/variable, then instance field) *before* falling back to a sibling `Property Get` of the same name, instead of the original order | Live testing exposed the original order's bug directly: `Property Let V(v As Long)` -- a common pattern, since a property's own value parameter is often named after the property -- read `v` as `0` instead of the just-bound parameter, because the sibling-`Property-Get` check for "v" ran first and recursively called `Property Get V()` instead of reading the local parameter | Matches ordinary lexical scoping (local shadows outer); verified with the same collision (`Property Let V(v As Long)`) both through `obj.V = x` and an unqualified sibling write | `REQ-0205` |
| Switch `scan_classes` from a single pass (scan each class's body immediately after registering its name) to two passes (register every class's name first, then scan every body) | A class-typed field/return type (`As SomeClass`) needs to look `SomeClass` up in `class_definitions_` while scanning the *referencing* class's own body; a single pass only has names registered for classes that appeared earlier among the `--class` arguments, silently failing to resolve a forward reference | Verified directly: `--class Holder "Public c As Counter" --class Counter "Public n As Long"` (`Holder` first, referencing `Counter` declared after it) resolves correctly | `REQ-0205` |
| Give a class-typed/`Object`-typed field or return-value slot no new tracking mechanism, reusing `Scope`'s existing `object_variables`/`object_class_names` sets (already used for `Dim x As Object`/`As SomeClass` variables and parameters) | The existing machinery already does exactly what an object-typed field/return slot needs -- fixed-type, `Nothing`-initialized, `Set`-only assignment, optional exact-class checking -- since `InstanceData::fields` and a call frame are both already a `Scope` | Zero new diagnostic codes or assignment logic needed for this half of the increment; `parse_set_statement`'s and `parse_member_set_assignment`'s object-reference-assignment logic was further unified into one shared `assign_object_reference` helper while making this change | `REQ-0205` |
| Store `Static` variable persistence on the owning `ProcedureDef` itself (a new `mutable Scope statics` member) rather than in any per-call `Scope`, with a save/restored `current_procedure_def_` pointer identifying "the currently executing procedure" for a `Static` statement to find its own storage | `ProcedureDef` instances live in `procedures_`/`class_def.methods` for the whole program and are never moved or erased once scanned (`unordered_map` reference stability), making them a natural, already-available home for state that must outlive any single call's transient frame | `mutable` lets every existing `const ProcedureDef&` call site keep working unchanged; verified with three successive calls to a `Function` incrementing a `Static` counter, observing `1`, `2`, `3` | `REQ-0206` |
| Copy a `Static` variable's value out of the frame and into persistent storage one variable at a time (via a new `Scope::static_variable_names` set), rather than trying to alias the frame's slot directly to the persistent one | A `Scope::variables` map stores `Value`s inline, not by reference, so there is no way for a frame's slot to *be* the persistent slot without a larger redesign; copy-in at the `Static` statement and copy-out just before the frame is discarded is a smaller, correct-enough substitute | Matches the value-copy-in/copy-out convention already established for ByRef parameter write-back in the very same function | `REQ-0206` |
| Enforce `Private` per-*class*, not per-instance -- a `Foo` method may reach *any* `Foo` instance's `Private` members, not only `Me`'s own -- via a new `member_accessible` check comparing `current_class_def()` (the class whose method is currently executing, if any) against the target's own class, rather than comparing instance identity | Matches real VB6 exactly, and was actually the *simpler* of the two designs once identified: it needs no new tracking at all, only a pointer comparison against machinery (`current_class_def()`) `REQ-0203`'s unqualified-sibling-call fallback had already introduced | Verified both directions: one `Foo` instance's method reading a *different* `Foo` instance's `Private` field (allowed) and a `Bar` method reading a `Foo` instance's `Private` field through a class-typed field it holds (rejected, `WFC0142`) | `REQ-0206` |
| Make a bare `Dim` class field implicitly `Private` (not `Public`, as `REQ-0203`'s original "visibility unmodeled, everything reachable" simplification effectively made every field) once real `Public`/`Private` enforcement existed to make the distinction meaningful | Matches real VB6's own module-level default exactly (a bare `Dim`, in *any* module type, is private to that module); confirmed no existing test relied on a `Dim`-declared class field being externally reachable before making this change | A deliberate, disclosed behavior change from `REQ-0203`'s original simplification, not merely an addition -- existing class-module source written against this evaluator using bare `Dim` for an externally-facing field would need to switch to `Public` | `REQ-0206` |
| Do not make `IsMissing` meaningful for an omitted `Optional Variant` parameter with no default | Real VB6's own `IsMissing` only distinguishes this one narrow case (any other `Optional` parameter form always reports `False` even when omitted); implementing it would need tracking a per-parameter-per-call "was this specific argument omitted" bit entirely separate from the bound value, for a rarely-relied-on piece of introspection | `REQ-0176`'s existing `IsMissing`-stays-`False` scope boundary is left exactly as it was; recorded explicitly as a disclosed, deliberate non-implementation in `REQ-0206`'s Scope rather than an oversight | `REQ-0206` |
| Reverse the earlier `REQ-0195` decision to leave `Rnd` returning `Double`; narrow `rnd_value`'s result to `float` only at the final return point, rather than reworking the generator's own arithmetic to operate in `float` throughout | The owner directly named this exact deferred item next; narrowing only at the boundary is provably equivalent here specifically because the generator's arithmetic (`state / 2^24`) is a division by a power of two, which is exact/correctly-rounded in both precisions -- there is no meaningful "compute in float instead" version that could give a different answer | Verified against the same reference fingerprint (`0.7055475`) already recorded in `REQ-0194`; the `Single` rendering is in fact a closer match to the reference than the old `Double` rendering was, since VB6's real `Rnd` is genuinely `Single` and never produces the old double-precision noise digits | `REQ-0194` |

| Item | Effect | Response | Status or owner |
| --- | --- | --- | --- |
| `InStr` `Option Compare Text` case first used `expect_success` (single-statement helper) for a two-line program | One unit assertion failed (`WFC0001` — parser saw a program where a statement was expected) | Switched the multi-line case to `expect_program_success`; rebuilt and reran to green | Closed |
| Reserved `vbTextCompare` declaration test initially expected duplicate-name diagnostic `WFC0013` | One evaluator assertion failed; implementation correctly emitted the established reserved-keyword diagnostic `WFC0017` | Corrected the test expectation and reran all 36 cases successfully | Closed |
| Initial `CBool` tests concatenated Boolean results directly | Unit and CLI cases correctly emitted `WFC0020` because the current `&` contract accepts only String or Long operands | Changed unit cases to independent `Print` statements and the CLI case to explicit `CStr` conversions; reran all 43 cases successfully | Closed |
| CTest's regular-expression engine did not match the multi-line `CBool` CLI output | The CLI command produced the correct `True`/`False` lines, but its pass expression failed | Kept the CLI assertion single-line through `CStr(CBool(...))` and restored an exact output expression | Closed |
| `parse_procedure_declaration_skip` jumped `offset_` past a declaration's trailing line break (not just past "End Sub"/"End Function" itself) | The very next top-level statement after a skipped declaration failed with `WFC0004` "unexpected trailing input", since `evaluate()`'s main loop still tried to consume a statement separator that was already gone | Changed `skip_to_matching_end` to leave the cursor right after "End Sub"/"End Function", not past its line break, matching every other statement handler's convention of leaving separator-consumption to the caller | Closed |
| `ByRef` argument write-back silently did nothing, and self-recursive calls segfaulted | Both traced to the same root cause: `std::vector<Scope>::push_back` reallocating on growth can copy (not move) existing `Scope` elements, orphaning the `Value*` pointer a `ByRef` argument captured before the call pushed its new frame | Switched `scopes_` from `std::vector<Scope>` to `std::deque<Scope>`, which guarantees push/pop at the ends never invalidates references or pointers to existing elements; verified with the same `ByRef` and recursive-`Factorial` cases | Closed |
| `Property Let V(v As Long)` read its own value parameter as `0` instead of the bound argument, whenever the parameter's name matched the property's own name | `parse_primary_base`'s unqualified-sibling-`Property-Get` fallback ran *before* `find_variable`, so reading `v` inside the Let body recursively called `Property Get V()` (returning the field's still-unchanged value) instead of resolving the local parameter `v` | Reordered the check to run only after `find_variable` finds nothing, so a local parameter/variable always shadows a same-named property; verified with `b.V = 99` and an unqualified `V = 42` from a sibling method, both correctly updating the backing field | Closed |
| An old unit test asserted `WFC0130` ("Property Get does not accept parameters") for a parameterized `Property Get` | That restriction was deliberately lifted this same increment (indexed properties), so the assertion now correctly failed with no error at all | Removed the stale assertion (the same scenario is now covered by this increment's own indexed-property success cases) | Closed |
| The first attempt at `obj.Name(args)` for an indexed `Property Get` reported `WFC0135` "unknown member" | `parse_member_access_after_dot`'s `(`-branch only checked `class_def.methods`, calling `call_class_method` unconditionally and erroring immediately when the name was a property, not a method | Added a `property_get` fallback (with its own argument-list parsing) alongside the method-call branch, mirrored in the unqualified-sibling-call `(`-branch in `parse_primary_base` for the same case with no `.` at all | Closed |
| `ParamArray nums() As Long` reported `WFC0141` "requires an explicit element type" even though one was clearly written | The new `ParamArray` element-type parser called `consume_keyword("as")` then `parse_type_keyword()` directly, with no `skip_horizontal_whitespace()` between them -- unlike every other `As Type` call site in the file -- so `parse_type_keyword()`'s own `consume_keyword("long")` saw a leading space, not `l`, and failed | Added the missing `skip_horizontal_whitespace()` between the two calls; verified with the exact reported case (`ParamArray nums() As Long`) and the full `Total`/`Sum2` test functions | Closed |
| `Total()` (a `ParamArray`-only call with zero extra arguments) crashed the process (exit code 3) instead of returning `0` | `parse_array_index`'s pre-existing `!execute_` (dry-run/type-check) short-circuit called `.front()` on the array unconditionally; a `For i = LBound(nums) To UBound(nums)` loop over an empty array (`0 To -1`) still type-checks its body once with `execute_ = false`, and `nums(i)` inside it hit `.front()` on a genuinely empty `std::vector` -- undefined behavior that had never been reachable before, since a `Dim`-declared array's size is always at least 1 | Return a placeholder `Long` instead of `.front()` when `array.elements` is empty in that dry-run-only path (the real value is discarded there regardless); verified `Total()` now returns `0` without crashing, alongside `Total(1, 2, 3, 4)` still returning `10` | Closed |
| Writing to element 0 of an unallocated dynamic array (`Dim arr() As Long: arr(0) = 1`) would have crashed the process | `parse_array_element_assignment` called `array.elements.front().index()` unconditionally to find the element type for its type-mismatch check; a dynamic array's `elements` is genuinely empty (not just transiently, inside a dry run, like the pre-existing `ParamArray` case above) until its first `ReDim` -- undefined behavior on an empty `std::vector`, discovered while implementing `REQ-0207`, before any test exercised the unallocated-write path | Replaced with `array.element_type_index` (the array's declared type, always available regardless of `elements.size()`), the same fix applied to the two `TypeName`/`VarType` `.front()` calls found by the same audit | Closed |
| `TypeName`/`VarType` on an empty `ParamArray` (called with zero extra arguments) would have crashed, and on a *non-empty* one would have reported the wrong type once the crash was naively fixed with a placeholder | Same root cause as the write-path bug above (`.elements.front()` unconditionally), plus a second latent gap the fix exposed: the `ParamArray`-construction site never set an element-type tag at all, so a placeholder fix would have made every `ParamArray`'s `TypeName` wrong, not just the empty one's | Set `element_type_index` from `param_array_parameter.type_index` (the `ParamArray`'s own declared element type) at construction, then read it via `array_element_default` in `TypeName`/`VarType` instead of inspecting a current element | Closed |
| A `ReDim`-grown slot of a `Variant`/`Object`-element array would have been seeded with `Integer` `0` instead of `Empty`/`Nothing` | `array_element_default` (used by `ReDim` to fill newly-created slots) had no case for either type -- it only covered the seven fixed scalar array-element types, since a `Variant`/`Object`-element array did not exist when it was first written | Added `Empty`/`Nothing` cases, keyed by their own `Value::index()` the same way every other case already is; unambiguous, since neither index can collide with any fixed scalar array's element type | Closed |

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
| Tests at current checkpoint | 96 | Local x64 CTest after `29a1905` |
| Tests at current checkpoint | 97 | Local x64 CTest after `c5f87c3` |
| Tests at current checkpoint | 98 | Local x64 CTest after `e278494` |
| Tests at current checkpoint | 99 | Local x64 CTest after `25e6c63` |
| Tests at current checkpoint | 100 | Local x64 CTest after `66a5114` |
| Tests at current checkpoint | 101 | Local x64 CTest after `b04f630` |
| Tests at current checkpoint | 102 | Local x64 CTest after `f3630b6` |
| Tests at current checkpoint | 103 | Local x64 CTest after `027d3cc` |
| Tests at current checkpoint | 104 | Local x64 CTest after `40cdcbe` |
| Tests at current checkpoint | 105 | Local x64 CTest after `1b9628c` |
| Tests at current checkpoint | 106 | Local x64 CTest after `4e4dc67` |
| Tests at current checkpoint | 107 | Local x64 CTest after `60ae221` |
| Tests at current checkpoint | 108 | Local x64 CTest after `45365b1` |
| Tests at current checkpoint | 109 | Local x64 CTest after `c4498ac` |
| Tests at current checkpoint | 110 | Local x64 CTest after `9ef2f61` |

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
| Integer numeric value type (increment #83) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Fixed-size arrays and minimal object stub (increment #84) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| User-defined Sub/Function procedures (increment #85) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Class modules foundation (increment #86) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Me keyword and Class_Initialize/Class_Terminate (increment #87) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Class-module refinements (increment #88) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Optional/ParamArray/Static/visibility (increment #89) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Rnd Single return type (increment #90) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Decimal promotion-order correction (increment #91) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Dynamic arrays / ReDim / ReDim Preserve (increment #92) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Erase / For Each (increment #93) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Multi-dimensional arrays (increment #94) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Array-typed parameters (increment #95) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Variant/Object-element arrays (increment #96) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Parenthesis-free niladic calls (increment #97) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Class-typed arrays / Variant-Object array parameters / array return type (increment #98) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Format Currency style (increment #99) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Bare-statement/obj.Method parenless calls (increment #100) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Format custom numeric picture strings (increment #101) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Dynamic multi-dimensional arrays (increment #102) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Format multi-section custom numeric pictures (increment #103) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Format custom picture `\` escape character (increment #104) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Format custom picture quoted literal text (increment #105) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Format custom picture `%` scaling (increment #106) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| IsMissing for omitted Optional Variant argument (increment #107) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Unconditional Do...Loop (increment #108) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Option Base (increment #109) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Inferred-type constants (increment #110) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Class-typed and generic Object parameters (increment #111) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Member access on Nothing dead-branch fix (increment #112) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Comma-separated Next attempt and revert (increment #113) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Static Variant object-lifetime fixes (increment #114) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |
| Static arrays and class-typed/Object locals (increment #115) | Not reported | Not reported | Live goal telemetry unavailable; no estimate recorded |

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
could not be probed live this session (tooling became unavailable) and was a
disclosed, reasoned-but-unverified choice at the time (see the Decisions
table and `REQ-0198`'s Scope) -- resolved by increment #91 below, which also
found `Decimal` dominates `Double` and `Currency`.

**Integer numeric value type (increment #83, `REQ-0199`):** adds the
distinct 16-bit `Integer` type — VB6's own `Integer`, not to be confused
with this codebase's pre-existing `Integer` C++ alias for `Long` — as a new
`Int16` (`std::int16_t`) `Value` alternative. Covers the `%` literal suffix
(range-checked -32768 through 32767, rejecting a fractional/exponent form)
and identifier character, `Dim`/`Const As Integer`, checked narrowing from
`Long`/`Single`/`Currency`/`Double` on assignment (banker's rounding for the
floating-point sources), and exact `Long` widening on assignment. Extends
the numeric promotion order to six levels,
`Integer < Long < Currency < Single < Decimal < Double`: `Integer`+
`Integer` computes exact checked 16-bit arithmetic (a new
`short_integer_binary`, mirroring the existing `integer_binary` for `Long`);
a mixed `Integer`/`Long` operand pair widens the `Integer` side exactly and
computes exact checked `Long` arithmetic (reusing `integer_binary`); any
purely-integral combination under `/` still promotes to `Double`, matching
the existing `Long`/`Long`-under-`/` rule. `\`/`Mod` continue to coerce any
numeric operand (now including `Integer`) to `Long` before dividing,
unchanged. `CInt` now returns a genuine `Integer` rather than a
`Long`-typed value narrowed to the `Integer` range (the same kind of change
`REQ-0195` made to `CSng`), and `CLng`/`CByte`/`CBool`/`CStr`/`CDbl`/`CSng`/
`CCur`/`CDec`/`IsNumeric`/`Abs`/`Int`/`Fix`/`Round`/`Str`/`Hex`/`Oct`/
`TypeName`/`VarType` all extend to accept an `Integer` argument. `Choose`'s
index, `QBColor`'s color index, `RGB`'s components, and other
strictly-`Long`-parameter intrinsics were confirmed to already exclude
`Single`/`Currency`/`Decimal` too, so they are left unextended, matching
that existing precedent rather than a gap specific to this increment.

**Fixed-size arrays and minimal object stub (increment #84, `REQ-0201`/
`REQ-0200`):** adds a new, genuinely recursive `ArrayValue{std::vector<Value>
elements, Integer lower_bound}` `Value` alternative for fixed-size
one-dimensional arrays: `Dim arr(n)`/`Dim arr(lo To hi) As Type` (`Type` one
of the existing fixed scalar types), indexed read (`parse_array_index`, a
new branch in `parse_primary`) and write (`parse_array_element_assignment`,
dispatched from a new `parse_assignment_or_array_element` wrapper at both
statement-parsing call sites), bounds-checked with new `WFC0111`, `LBound`/
`UBound`, and real `IsArray`/`TypeName`/`VarType` results (`"Long()"`/
`8195` for a `Long` array, matching VB6's element-VarType-ORed-with-
`vbArray` convention). Also adds a minimal object-reference stub: `Nothing`
as a new distinct `Value` state (the only object value this evaluator can
produce, since it has no class modules or `New`), `Dim x As Object` (a
fixed, non-retyping type, tracked via a new `object_variables_` set
mirroring `variant_variables_`), a new `Set` statement as the only legal
way to assign an object reference (new `WFC0106`/`WFC0108`/`WFC0109`), the
`Is` operator for object identity (new `WFC0107`, also used to reject plain
`=`/`<>`/etc. on an object reference, matching real VB6), and real
`IsObject`/`TypeName`/`VarType` results. Both features were scoped via two
`AskUserQuestion` calls after the owner's bare instruction ("Fixed-size 1-D
arrays only" / "Minimal object stub") rather than assumed, since each could
otherwise range from a small addition to a multi-session architecture
effort. Closed the resulting exhaustive-elimination crash sites for the two
new `Value` alternatives across `render`, concatenation, every `CXxx`
conversion, `IsNumeric`, and the `Select Case ... To` range form (the same
audit pattern applied to every prior new `Value` alternative this session).

**User-defined Sub/Function procedures (increment #85, `REQ-0202`):** the
owner's next instruction, "finish objects," turned out to depend on
something that did not exist at all: user-defined procedures (`REQ-0141`
had explicitly deferred "procedure scope" to "later requirements" since
MP-0002's second increment, and nothing since had added it). Rather than
assume scope, this was surfaced back to the owner via `AskUserQuestion`,
which chose to build procedures first, as their own increment, with no
class-module layer yet. Adds module-level `Sub Name(params) ... End Sub`/
`Function Name(params) As Type ... End Function` declarations: a new
lightweight pre-scan pass (`scan_procedures`, run once before the module's
top-to-bottom execution begins) finds every declaration and its body range
so calls resolve regardless of textual order (forward reference, direct and
mutual recursion), while the main execution pass skips over each
declaration where it is written. `ByVal`/`ByRef` parameters (`ByRef` is
VB6's unwritten default); a `ByRef` argument whose source text is a single
bare variable name gets its parameter's final value copied back after the
call, while any other argument form behaves as `ByVal` (no caller-visible
variable to write back into). A `Function` returns via assignment to its
own name, like an implicit local variable. `Exit Sub`/`Exit Function`
(new `WFC0124`/`WFC0125`), a nesting-depth guard (new `WFC0123`, see below),
and the `Call` statement (the only supported way to invoke a `Sub`
statement-style, matching real VB6; this evaluator still does not support
VB6's parenthesis-free `Name arg1, arg2` call form).

Implementing real per-call local variable scope required refactoring the
four flat `variables_`/`constants_`/`variant_variables_`/`object_variables_`
members (used throughout the evaluator since the Variant/Object increments)
into a `Scope` struct plus a `scopes_` scope-chain, with lookups checking
only the current call's own frame and the module frame -- never an
intermediate caller's frame, matching VB6's own module/procedure two-level
scoping. This was a ~28-call-site mechanical refactor, done and verified
against the full existing test suite *before* writing any new call logic on
top of it.

Live testing during this same increment surfaced two real bugs, both
traced to one root cause: `ByRef` argument write-back silently did nothing,
and self-recursive calls segfaulted. `std::vector<Scope>::push_back`
reallocating on growth can *copy* (not move) existing `Scope` elements when
the move constructor isn't usable without an exception guarantee,
orphaning a `Value*` pointer a `ByRef` argument had captured before the
call pushed its new frame onto the vector. Switched `scopes_` from
`std::vector<Scope>` to `std::deque<Scope>`, which guarantees push/pop at
the container's ends never invalidates references or pointers to existing
elements -- fixing both symptoms. A related, unguarded-recursion stack
overflow was independently observed (crashing a local debug build somewhere
between 100 and 128 nested calls), addressed with a `WFC0123` depth guard
at 64 levels (chosen empirically for a comfortable safety margin below the
observed crash range).

**Class modules foundation (increment #86, `REQ-0203`):** the owner chose
"Class modules foundation" from this log's own "Remaining next increments"
list (via `AskUserQuestion`), then three further `AskUserQuestion` choices
scoped it: classes supplied as separate sources alongside the standard
module (new CLI `--class <Name> <source>`, mirroring a real VB6 project's
separate `.cls` files, over an inline `Class...End Class` block) with
fields, methods, *and* `Property Get`/`Let`/`Set` accessors together (over
fields/methods alone), and `New`-only instantiation (no
`Class_Initialize`/`Class_Terminate`). Adds a new `ObjectInstance` `Value`
alternative -- a `shared_ptr<InstanceData>` handle (`InstanceData`:
`{class_name, Scope fields}`, reusing `Scope` for per-instance field
storage) -- giving `Is`/`Set`/`TypeName`/`VarType`/`IsObject` a real,
non-`Nothing` object value for the first time since `REQ-0200`'s stub;
every one of that requirement's `holds_alternative<Nothing>` rejection
sites (`render`, `&` concatenation, `compare`, every `CXxx` conversion,
`IsNumeric`) was audited and extended to reject an `ObjectInstance` the
same way, since none of them are compiler-enforced exhaustive `std::visit`
matches. A class's own source is scanned by a new `scan_class_body`
(mirroring `scan_procedures`), and a class method/property's body executes
against its own class's source (`source_`/`offset_` are swapped in and
back around the call, exactly like `execute_` already is) and its own
instance's field scope (a new `instance_scopes_` stack `find_variable`
consults instead of the real module scope, so a class is isolated from the
standard module exactly like a second, separate VB6 module would be).
`.member` access chains through a new `parse_primary` postfix loop (so a
`Variant` field/return value that happens to hold another instance chains
further, `a.b.c`, with no class-typed-field-specific code needed for it).

Live testing during this same increment surfaced one real, since-fixed
bug: a method calling a sibling method of its own class unqualified
(including calling itself, for recursion) failed with "unsupported
function". `Me` is not implemented, so nothing else lets a class call its
own members at all without this; the root cause was that `instance_scopes_`
was originally a `Scope*` stack (enough for field lookups) with no way to
recover the current instance's *class* to look a sibling method up in --
fixed by switching it to an `InstanceData*` stack, then adding the
unqualified-sibling-call/read fallback to `parse_primary_base`/
`parse_call_statement` (see the Decisions table above for the full
before/after). Confirmed with a self-recursive `Factorial` method and a
second method calling it, both unqualified.

**Me keyword and Class_Initialize/Class_Terminate (increment #87,
`REQ-0204`):** the owner's next instruction named exactly the two gaps
`REQ-0203`'s own Scope section had just deferred ("do the me keyword class
initailize and terminate"). `Me` is now a reserved keyword, valid only
inside a class member's body (`WFC0138` outside one), producing a fresh
`ObjectInstance` that shares the current call's own instance identity via
a new `InstanceData : enable_shared_from_this<InstanceData>` base (plain
raw-pointer wrapping would have created a second, independent `shared_ptr`
control block -- a double-free once both reached zero). `Me` slots into
the existing postfix `.` chain and the `Me`-specific branches added to
`parse_assignment_or_array_element`/`parse_call_statement`/
`parse_set_statement`, so `Me.Field`, `Me.Method(args)`,
`Call Me.Method(args)`, `Set Me.Property = expr`, and passing/returning
`Me` itself (`Set x = Me`, `Set GetSelf = Me` from an `As Variant`
`Function`) all work the same way the equivalent access through an
ordinary object-reference variable already did.

`Class_Initialize` (an optional, parameterless `Sub`) runs from
`instantiate_class` immediately after every field is zero-initialized and
before the new instance is handed back -- a plain nested call with no
lifetime hazard, since it happens mid-expression-evaluation, never inside
any container's own teardown. `Class_Terminate` needed more care: the
first design considered, a destructor hook on `InstanceData` firing
whenever its `shared_ptr` refcount reaches zero, was rejected after
tracing through exactly when that refcount can hit zero -- including from
*inside* a `Scope`'s `variables` map destroying its own `Value`s as part
of a call frame's `scopes_.pop_back()`, or the `Interpreter`'s own final
member destruction at the very end of the program. Reentrantly calling
back into the evaluator's mutable state (pushing a new call frame onto a
`scopes_` deque that is itself mid-`pop_back()`) from inside that teardown
is undefined behavior, not merely awkward, so no destructor hook was
added. Instead, a new `terminate_if_last_reference`/`drain_scope_instances`
pair fires `Class_Terminate` only from three explicit, always-safe points:
a `Set` overwrite (including `Set x = Nothing`), a call frame's own locals
as `invoke_definition` returns (draining one variable at a time -- checking
`use_count() == 1` and clearing to `Empty` as it goes, not pre-scanning
all of them first, so two same-frame aliases of the same instance still
terminate it exactly once instead of never), and the module scope at the
end of a successful program (draining before `output_` is moved out, so a
`Print` inside `Class_Terminate` still reaches the final output). Verified
directly: `Set x = Nothing` on a module-level variable, a `Sub`-local
instance never explicitly cleared, a module-level instance surviving to
program end, and the same-frame-alias case all fire `Class_Terminate`
exactly once, in the right place.

**Class-module refinements (increment #88, `REQ-0205`):** the owner
selected three items (of four offered via `AskUserQuestion`) from this same
log's own "Remaining next increments" list: unqualified sibling
`Property Let`/`Set` writes, class-typed/`As Object` fields and return
types, and indexed `Property` accessors. Unqualified writes mirror the
existing unqualified read/call fallback exactly, added symmetrically to
`parse_assignment` (`Property Let`) and `parse_set_statement`
(`Property Set`). Class-typed/`Object`-typed fields and `Function`/
`Property Get` return types needed no new assignment machinery at all --
`Scope`'s existing `object_variables`/`object_class_names` sets (already
built for `Dim x As Object`/`As SomeClass`) apply unchanged to
`InstanceData::fields` and a call frame's return-value slot, since both are
already a `Scope`; `parse_set_statement`'s and `parse_member_set_
assignment`'s object-assignment logic were unified into one shared
`assign_object_reference` helper along the way. Resolving `As SomeClass`
against a class declared *later* on the `--class` command line required
switching `scan_classes` from one pass to two (register every class name
first, then scan every body), otherwise a forward reference would see an
unregistered name. Indexed properties relaxed `Property Get`'s former
zero-parameter requirement and `Property Let`/`Set`'s former
exactly-one-parameter requirement, with the value parameter always last;
`obj.Name(args)` reads and `obj.Name(args) = expr`/`Set obj.Name(args) =
expr` writes reuse the exact same parenthesized-argument-list parsing a
method call already uses, via a new shared `invoke_property_let_or_set`
helper.

Live testing during this same increment surfaced one real, since-fixed
bug, found while testing the *first* item (unqualified writes) rather than
anything indexed-property-specific: `Property Let V(v As Long)` -- a
common real-world pattern, naming a property's own value parameter after
the property -- read its own parameter `v` as `0` instead of the value
just bound to it. The cause: `parse_primary_base`'s existing
unqualified-sibling-`Property-Get` fallback (added under `REQ-0203`) ran
*before* the local-parameter/variable lookup, so reading `v` recursively
called `Property Get V()` (returning the backing field's still-stale
value) instead of resolving the local parameter. Reordering the check to
run only once `find_variable` finds nothing fixed it, matching ordinary
lexical scoping (local shadows outer) -- the same reordering that had
already been applied correctly, from the start, to the new unqualified
`Property Let`/`Set` write paths this same increment added.

**Optional/ParamArray/Static/visibility (increment #89, `REQ-0206`):** the
fourth item selected alongside `REQ-0205` (of four offered via
`AskUserQuestion`) from this same log's own "Remaining next increments"
list -- a procedure-system increment rather than a class-specific one,
applying uniformly to module-level procedures and class members.
`Optional [As Type] [= default]` parameters relax the previous exact-arity
requirement to a range, filling an omitted trailing argument with `default`
(scanned as a constant expression, mirroring `Const`) or the type's zero
value. `ParamArray name() As Type` collects every remaining call argument
into a fresh zero-based `ArrayValue` -- reusing `REQ-0201`'s existing array
type completely unchanged, including a genuinely empty array (`LBound = 0`,
`UBound = -1`) when called with zero extra arguments. `Static name [As
Type]` persists a local's value across separate calls by storing it on a
new `mutable Scope statics` member added directly to `ProcedureDef` (found
once at scan time and never moved afterward, making it a natural home for
state outlasting any one call's frame), located via a new
save/restored `current_procedure_def_` pointer; each call copies the
persisted value into its own frame at the `Static` statement and copies it
back out just before the frame is discarded.

`Public`/`Private` visibility is the change with the widest blast radius:
it corrects `REQ-0203`'s original "not modeled, everything reachable"
simplification, making a bare `Dim` class field implicitly `Private`
(matching real VB6's own default) rather than externally accessible like
`Public`. Enforcement is per-*class*, not per-instance -- a new
`member_accessible` check compares `current_class_def()` (the class whose
method is currently executing, if any) against the target member's own
class, so one `Foo` instance's method can reach *any* `Foo` instance's
`Private` members, while module-level code or a different class's method
cannot. `Public`/`Private` is also accepted (but not enforced) before a
module-level `Sub`/`Function`, since this evaluator has only one standard
module.

Live testing during this same increment surfaced two real, since-fixed
bugs, both found while exercising `ParamArray` rather than anything
`Optional`/`Static`/visibility-specific. First, `ParamArray nums() As Long`
itself failed to parse ("requires an explicit element type" despite one
being written): the new element-type parser called `consume_keyword("as")`
then `parse_type_keyword()` back-to-back with no `skip_horizontal_
whitespace()` between them, unlike every other `As Type` call site in the
file, so `parse_type_keyword()`'s own keyword match saw a leading space
instead of the type name's first letter and failed. Second, and more
serious: calling a `ParamArray`-only function with zero extra arguments
(`Total()`) crashed the process outright. The cause was unrelated to
anything new in `ParamArray`'s own binding logic -- it was a latent bug in
`parse_array_index`'s pre-existing `!execute_` (dry-run/type-check)
short-circuit, which called `.front()` on the array unconditionally. A
`For i = LBound(nums) To UBound(nums)` loop over a genuinely empty array
(`0 To -1`) still type-checks its body once with `execute_ = false`, and
indexing `nums(i)` inside that dry run hit `.front()` on an empty
`std::vector` -- undefined behavior that had simply never been reachable
before this increment, since a `Dim`-declared array's size is always at
least 1. Fixed by returning a placeholder value instead of `.front()` when
the array is empty in that dry-run-only path (the real value there is
always discarded regardless).

**Rnd Single return type (increment #90):** the owner pointed directly at
one of two items this same log's "Remaining next increments" list had just
recorded as deliberately deferred. `Rnd` now returns a genuine `Single`
instead of `Double`, closing the gap `REQ-0194` disclosed back when `Single`
did not exist yet and `REQ-0195` then explicitly declined to revisit. The
fix is a single narrowing at the point of return (`static_cast<float>(
rnd_value(rnd_state_))`), not a rework of the generator's own arithmetic:
`state / 2^24` is a division by a power of two, which is exact/correctly-
rounded in both `float` and `double`, so narrowing the already-computed
double result is provably identical to computing the same division in
single precision from the start. Every existing `Rnd`/`Randomize` test's
expected output string got shorter as a result -- `0.7055475` instead of
`0.7055475115776062` -- since the extra digits were always double-precision
noise the reference VB6 runtime (whose `Rnd` is genuinely `Single`) never
actually produces; the `Single` rendering is a strictly closer match to the
already-recorded reference fingerprint than the old `Double` rendering was.

**Decimal promotion-order correction (increment #91, `REQ-0198`):** the
owner's remaining pointed-at item, "verifying the Decimal-vs-Single
promotion order," needed a live VB6 probe rather than a code change (owner:
"ask for vb6 again please I was afk," after an earlier `request_access`
attempt was denied while the owner was away). The probe confirmed `Decimal`
dominates `Single` as already coded, but also revealed `Decimal` dominates
`Double` -- at both small and large magnitude -- plus `Currency` and `Long`
(see the Reference Probe Evidence section above), directly contradicting the
existing `NumericCategory` comment's claim that "Double dominates every
other numeric type" was independently justified. That claim had never
actually been verified against the reference runtime; it was a reasoned
assumption based on `Double` having the largest representable magnitude,
which turns out not to be how real VB6 resolves mixed `Decimal` arithmetic
(it stays in `Decimal`'s exact fixed-point representation instead). Fixed by
reordering the `NumericCategory` enum so `decimal_precision` sorts above
`double_precision`; the arithmetic branch's existing `to_decimal` helper
already had a generic `as_double`-based fallback for every category with no
dedicated conversion, so `Double` operands fall into the same path `Int16`/
`Single` already used, with no new conversion logic required. Updated
`REQ-0198`'s Requirement and Scope sections to state the now-fully-verified
six-way order and removed the stale "unverified against Single" disclosure;
added unit tests covering `Decimal` dominance over `Single`/`Currency`/
`Long`/`Double` in both operand orders.

**Dynamic arrays / ReDim / ReDim Preserve (increment #92, `REQ-0207`):** the
owner pasted two "Remaining next increments" bullets from this same log
(multi-dimensional/`Erase`/`For Each`/array-parameters, and Variant/Object-
element arrays); `AskUserQuestion` offered four escalating scope options and
the owner chose the smallest, "ReDim/Preserve only." `Dim identifier()` (no
bound) now declares a dynamic array, unallocated until `ReDim`, instead of
reporting `WFC0116` (retired); the new `ReDim [Preserve] identifier(<bound>)`
statement reuses `REQ-0201`'s bound-expression grammar and `WFC0117`/
`WFC0115` diagnostics, adding new `WFC0145` for a target that is not a
previously `Dim identifier()`-declared dynamic array. Unlike `Dim`/`Const`/
`Static`, `ReDim` is an ordinary executable statement (not gated by
`allow_declarations_`), so it works inside `If`/`While`/`Do`/`For` blocks.
`LBound`/`UBound`/indexing on an unallocated array reuses the existing
`WFC0111`, matching real VB6's identical run-time error 9 for both an
out-of-bounds index and an unallocated array -- verified distinct from a
legitimately zero-length *allocated* array (an empty `ParamArray`), which
still answers `UBound < LBound` without error.

`ArrayValue` gained `is_dynamic`/`is_allocated`/`element_type_index` fields.
The last stores a `Value::index()`, not a `Value` itself: `Value` is
`std::variant<..., ArrayValue, ...>`, so a `Value` held directly inside
`ArrayValue` (unlike the existing `std::vector<Value> elements`, whose
heap indirection breaks the cycle) would make `ArrayValue` and `Value`
recursively depend on each other's complete size -- unbuildable, not merely
inelegant. A new `array_element_default(type_index)` free function
reconstructs the type's zero value on demand.

Auditing every `.front()` call this change made newly reachable on a
genuinely (not just transiently) empty array found two real crash risks
beyond `ReDim` itself, both fixed the same way: `parse_array_element_
assignment` read `array.elements.front().index()` unconditionally for its
type-mismatch check (crashes writing to an unallocated array's element 0),
and `TypeName`/`VarType` did the same for the array's type-name/VarType
code. A third, subtler gap surfaced while fixing the second: the
`ParamArray`-construction site never set an element-type tag at all, so a
naive placeholder fix would have made `TypeName`/`VarType` report the wrong
type for *every* `ParamArray`, not just an empty one -- fixed by setting
`element_type_index` from the `ParamArray` parameter's own declared type at
construction.

**Erase / For Each (increment #93, `REQ-0208`/`REQ-0209`):** the owner
pasted back the exact remaining-items list from the previous increment's
closing summary (multi-dimensional arrays, `Erase`, `For Each`, array-typed
parameters, `Variant`/`Object`-element arrays) with no further comment,
read as a direct instruction to keep working through it (the established
pattern in this log for a pasted bullet list). Took `Erase` and `For Each`
first, as the two smallest, most self-contained items -- `Erase`
(`REQ-0208`) reuses `ArrayValue`'s existing `is_dynamic`/`is_allocated`/
`element_type_index` fields directly (reset in place for a fixed array,
deallocate for a dynamic one), and `For Each` (`REQ-0209`) reuses the
numeric `For`'s existing `parse_for_body`/`Exit For`/`for_depth_` loop-body
machinery unchanged, needing only its own control-variable-binding and
continuation logic. Both new diagnostics (`WFC0146` for `Erase`, `WFC0147`
for `For Each`) follow the same "one code per requirement, message varies
by branch" convention already used for `WFC0145` (`ReDim`). The remaining
three items (multi-dimensional arrays, array-typed parameters,
`Variant`/`Object`-element arrays) are each substantially larger and are
left for following increments.

**Multi-dimensional arrays (increment #94, `REQ-0210`):** the fourth item
from the owner's pasted remaining-increments list, and the largest of the
three left after `Erase`/`For Each`. Scoped unilaterally to "fixed-size
only" (documented in `REQ-0210`'s Scope) since a dynamic multi-dimensional
array needs its own "dimension count fixed on first ReDim" tracking, a
meaningfully separate feature from generalizing the existing fixed
declaration/indexing paths. `Dim identifier(b1, b2, ...) As Type` and
`identifier(i1, i2, ...)` reuse the existing 1-D bound/index grammar
per-dimension; `LBound`/`UBound` gained an optional 1-based dimension
argument (new `WFC0148`). `WFC0115`, which previously rejected any comma
in an index outright, now means "index count does not match the array's
declared dimension count" -- the same code, a now-accurate message, no
new diagnostic needed. `Erase` and `For Each` required *zero* code changes:
both already operate on `ArrayValue`'s flat `elements` field generically,
confirmed by testing each directly against a 2-D array; a row-major flat
layout (last dimension fastest) was chosen specifically so `For Each`'s
existing unmodified flat iteration visits elements in real VB6's own
order. The read/write index-parsing code came out shorter after adding
multi-dimensional support, not longer, by refactoring the shared
parsing/bounds-checking logic into two new helpers (`parse_index_list`,
`array_flat_offset`) used by both.

**Array-typed parameters (increment #95, `REQ-0211`):** the fifth and
final item from the owner's original pasted remaining-increments list.
`name() As Type` declares a dimension-count-agnostic, always-`ByRef`
array parameter; binding requires a bare-identifier array-variable
argument of matching element type (new `WFC0149` alongside the reused
`WFC0016`). The write-back half needed no new code at all -- it reuses
`REQ-0202`'s existing bare-identifier `ByRef` copy-back mechanism exactly
as already written, since an array parameter is simply never `by_val` and
always has a non-null `byref_target` by construction. Verified this
correctly propagates a `ReDim Preserve` performed *inside* the callee
back to the caller, not just element writes, since the whole `ArrayValue`
(new size and bounds included) is what gets copied back. `LBound`/
`UBound`/indexed access/`Erase`/`For Each` all work unchanged inside the
callee, extends to class methods for free (shared parameter parser), and
a multi-dimensional array binds the same way as a 1-D one. This closes
every item from the *first* of the owner's two originally-pasted
"Remaining next increments" bullets (`ReDim`/`ReDim Preserve`/
multi-dimensional arrays/`Erase`/`For Each`/array-typed parameters);
`Variant`/`Object`-element arrays, the second bullet, was the one item
from the owner's original pasted list not yet picked up -- closed by the
very next increment below.

**Variant- and Object-element arrays (increment #96, `REQ-0212`):** the
second and final item from the owner's original pasted remaining-
increments list. `Dim identifier(...) As Variant`/`As Object` now parses
for any array kind (fixed, dynamic, multi-dimensional) by simply dropping
the `!is_array &&` guard those two `As`-clause branches already had. A
`Variant` element retypes freely per element (mirroring a scalar
`Variant`'s rule); an `Object` element is `Set`-only (`WFC0108` on plain
`=`, mirroring a scalar `Object`'s rule) -- both checked via two new
`ArrayValue` fields, `is_variant_element`/`is_object_element`, rather
than adding the array's own *name* to the existing scope-level
`variant_variables`/`object_variables` sets, which would have wrongly let
a whole-array assignment retype the array away from being an array at
all (those sets govern a whole scalar/object *variable's* own rules, not
an array's per-element ones). Added `Set arrayName(index...) = expr`
parsing to `parse_set_statement`, which previously had no array-element
form at all, reusing `assign_object_reference` directly against the
computed element slot. Found and fixed a real bug surfaced by writing the
`ReDim`-refill test first: `array_element_default` had no `Empty`/
`Nothing` cases, so a `ReDim`-grown `Variant`/`Object` array's new slots
would have been seeded with a stray `Integer` `0` instead -- both cases
added, unambiguous since neither index collides with any fixed scalar
array's element type. `TypeName`/`VarType` special-case both new kinds to
report the array's own declared kind (`"Variant()"`/`"Object()"`) rather
than a nonsensical lookup through the now-inapplicable
`element_type_index`. Removed two now-stale `REQ-0201` tests asserting
`Dim arr(3) As Variant`/`As Object` fail with `WFC0012` -- that failure
is exactly what this increment changes. Deliberately left open, disclosed
in `REQ-0212`'s Scope: `Class_Terminate` does not proactively drain an
instance reachable only through an array element (mirroring `REQ-0204`'s
own existing field-cascade gap, not solved here), and `REQ-0211`'s array
parameters still require a concrete scalar element type, not `Variant`/
`Object`. This closes both bullets from the owner's original pasted
remaining-increments list in full.

**Parenthesis-free niladic calls (increment #97, `REQ-0213`):** with both
owner-pasted array bullets closed, `AskUserQuestion` offered four options
from the log's own remaining-increments list; the owner chose this one
over dynamic multi-dim arrays, small array-polish items, and `Format`'s
Currency style. `Print Rnd`, `x = NextId`, an unqualified bare sibling
class-Function reference, and `Call name` (zero arguments) all now work,
by changing two argument-parsing call sites (`parse_call_argument_list`,
and `parse_function_call`'s own inline loop) to treat a missing `(` as
zero arguments instead of an error, and adding two new checks --
known-procedure-name, then known-intrinsic-name -- to
`parse_primary_base`'s bare-identifier fallback, reached only once every
variable/`Const`/sibling-Property-Get possibility has already found
nothing. The intrinsic-name check works by speculatively calling
`parse_function_call` and checking whether it failed specifically with
`WFC0071` "unsupported function" *before consuming any input* (that
check runs before any argument parsing), clearing the speculative error
and falling through to the ordinary `WFC0015` "undeclared variable" in
that case -- avoiding a second, drift-prone copy of the intrinsic
dispatcher's ~40-name recognition list. Deliberately excluded, discovered
as real boundaries during manual verification rather than assumed up
front: a parenthesis-free call *with* arguments (`Foo 5, 6`, the classic
ambiguous form); a bare `Name` full statement with no `Call` keyword at
all, even for zero arguments; and `obj.Method`/`Call obj.Method` (dotted
access) without parentheses, which still only recognizes `Property Get`
in its own no-parens branch, never a method.

**Small array/class polish (increment #98, `REQ-0214`/`REQ-0215`/
`REQ-0216`):** closed the "small array/class polish items" bucket from
the log's own remaining-increments list in one increment, since all three
items were already fully itemized from the earlier `AskUserQuestion` and
needed no further scoping decision. Class-typed array elements
(`Dim arr(...) As SomeClass`) reuse `REQ-0212`'s generic `As Object`
element machinery unchanged, adding only a new `ArrayValue.
element_class_name` field threaded into the existing `assign_object_
reference`'s class-match check. `Variant`/`Object`-element array
parameters (`nums() As Variant`/`As Object`) reuse `REQ-0211`'s `ByRef`
write-back verbatim, needing only two new `ProcedureParameter` flags and
a widened element-kind-match check -- `parse_type_keyword` already parsed
`Variant`, the array-parameter branch was simply rejecting it before. The
array return type (`Function name(...) As Type()`) is the largest of the
three: the return-value slot now starts as an unallocated dynamic array
(`REQ-0207`) instead of a scalar zero, so the body can assign a whole
array to its own name or `ReDim`/`ReDim Preserve` it directly through
completely unmodified array machinery -- verified both forms, including
`ReDim`ing the return slot by name from inside the function body itself.
All three inherit `REQ-0201`'s pre-existing, disclosed whole-array-
assignment simplification rather than introducing new element-type
validation, confirmed (not assumed) by testing plain mismatched-element-
type array assignment directly before relying on it.

**Format Currency style (increment #99, updated `REQ-0193`):** the
smallest remaining item, and the only one not about arrays or
procedures -- picked to diversify after several array/procedure-focused
increments in a row. `REQ-0193` originally deferred `Currency` because no
`Currency` type existed yet; `REQ-0196` closed that blocker, leaving only
the locale-symbol design question `REQ-0193`'s own Scope had flagged.
Rather than defer further, this increment made a fixed, disclosed choice:
reuse `render_fixed_style`'s existing grouped/two-decimal "Standard"
magnitude unchanged, with a `$` inserted immediately before the digits
(after any leading `-`). Explicitly not a claim of matching real VB6's
own locale-dependent rendering (system currency symbol, commonly a
parenthesized negative convention) -- recorded as a reasoned-but-
unverified simplification, the same pattern already established for
other locale/reference gaps this evaluator has no way to resolve exactly
(e.g. `Rnd`'s `Randomize`-seed hash).

**Bare-statement/obj.Method parenless calls (increment #100, `REQ-0217`):**
closed both remaining gaps `REQ-0213`'s own Scope had flagged. A bare
`name` statement (no `Call`) needed only a lookahead in `parse_statement`'s
existing identifier fallback. `obj.Method` without parentheses turned out
to need fixing *two* separate dispatch points, not one, once actually
tested: `parse_member_access_after_dot` (reached by `Call obj.Method` and
any expression read) simply never checked `class_def.methods` in its
no-parens branch, a one-line fix; but the fully bare `obj.Method`
statement (no `Call` at all) routes through an entirely different
function, `parse_member_assignment`, which has no method-calling
capability whatsoever -- it exists solely for `obj.Prop = expr`. Fixing
that needed a lookahead of its own inside `parse_assignment_or_array_
element`'s object-dot branch. A confusing `WFC0004` "unexpected trailing
input" during manual verification turned out to be a self-inflicted red
herring: the test class used an array field (`Dim data(5) As Long`),
never valid syntax here since `REQ-0203`'s class fields are scalar-only
-- a pre-existing, unrelated boundary, not a regression, once isolated
and the test corrected.

**Format custom numeric picture strings (increment #101, `REQ-0218`):**
the last item picked from the log's own remaining-increments list before
this session's array/procedure/call work, deferring dynamic multi-dim
arrays and `ReDim`-as-implicit-declaration for a later pass. Any `Style`
not naming a `REQ-0193` reserved style is now a custom picture: `0`/`#`
digit placeholders, a single `.` splitting integer/fraction sections and
triggering the same nearest-even rounding `Fixed`/`Standard` already use
(the same `to_chars(..., chars_format::fixed, N)` call, generalized from
a hardcoded `2` to the picture's own fraction-placeholder count `N`), `,`
grouping among integer-section placeholders, and literal passthrough for
everything else. This retires `WFC0102` entirely -- real VB6 never
rejects a `Style` string outright, so every character is either a
recognized token or a literal, and the one existing test asserting the
old unconditional rejection was removed rather than updated, since its
input (`"Nope"`) is now valid (an all-literal picture). Also fixed, in
passing, a wrong requirement-number citation from increment #99's own
`Currency`-style comment (`REQ-0217` where `REQ-0193` was meant).

**Dynamic multi-dimensional arrays (increment #102, `REQ-0219`):** picked
from `REQ-0207`/`REQ-0210`'s own "Remaining next increments" bullets
(owner: "keep working until we are out of credits"), combining the two
requirements' foundations rather than adding a third parallel one.
`Dim arr(,)` (N commas, no bounds) pre-declares an N+1-dimension count
before allocation; a plain `Dim arr()` instead fixes its count from its
own first `ReDim`. Once fixed either way, every later `ReDim` must supply
exactly that many bounds (`WFC0115`), and `ReDim Preserve` may only
resize the *last* dimension (new `WFC0151`) -- both generalizing the
existing 1-D logic to N dimensions instead of duplicating it, so the 1-D
case is just N=1 of the same code path. Manual testing (not the unit
suite) found a real bug this way: `Erase` on a dynamic array cleared its
elements but not its per-dimension bounds, so a subsequent index on an
erased multi-dim array passed a stale bounds check and indexed past the
end of a now-empty `std::vector` -- undefined behavior, observed as a
hung `wfc.exe` process requiring `taskkill //F` to terminate rather than
a clean crash. Fixed by clearing `dimensions` in `Erase` and extracting
a shared `array_expected_dimension_count()` helper, applied at that call
site and three others that had each independently (and, for an
unallocated pre-declared array, incorrectly) computed the same value
inline. Re-ran the exact hanging repro under a `timeout` guard after the
fix to confirm it now completes promptly and reports `WFC0111`, before
writing a formal regression test for it. Also updated `REQ-0208`'s
Requirement/Scope and `REQ-0207`/`REQ-0210`'s Scope sections, since both
previously stated multi-dimensional arrays were dynamic-array-
incompatible or fixed-size-only.

**Format multi-section custom numeric pictures (increment #103,
`REQ-0220`):** picked from `REQ-0218`'s own "Remaining next increments"
bullet (owner: "keep working until we are out of credits"). Splits a
`Style` on `;` into up to three positive/negative/zero sections,
matching real VB6: two sections split positive-or-zero (first) from
negative (second); three add a dedicated zero section (third), chosen
ahead of the sign check whenever the value is exactly `0.0`. A negative
value's own section is passed its magnitude rather than the signed
value, so `REQ-0218`'s existing single-section helper's "negative ->
leading `-`" check simply never fires for it -- any sign shown comes
from the section's own literal characters (e.g. `"(0.00)"`'s
parentheses), matching real VB6's "the negative section owns its own
sign" rule, with no separate flag needed to suppress the automatic one.
Manually verifying this against the canonical real-world negative-
section idiom -- a parenthesized or currency-prefixed picture --
surfaced two genuine pre-existing bugs in `REQ-0218`'s single-section
rendering: overflow digits (more digits than placeholders) landed ahead
of the *entire* picture instead of next to the leftmost placeholder,
stranding a leading literal like `(` in the middle of the digits; and
comma grouping counted every character in the rendered integer section
from its end instead of only its digit positions, so a literal trailing
the last placeholder (a closing `)`) miscounted the grouping outright.
Both are now fixed (tracking where the leftmost placeholder landed, and
a parallel is-this-a-digit marker, respectively) and covered by
regression tests, with `REQ-0218`'s own Requirement/Scope updated to
describe the corrected behavior.

**Format custom picture `\` escape character (increment #104,
`REQ-0221`):** picked from `REQ-0218`'s own "Remaining next increments"
bullet (owner: "keep working until we are out of credits"). The
character right after a `\` is now always a literal -- `\0`/`\#` never a
digit placeholder, `\,` never the grouping-enable comma, `\.` never the
decimal-point separator, `\;` never a `REQ-0220` section separator, and
`\\` a single literal backslash. Implemented by expanding every escaped
pair ahead of time into a parallel `(text, forced_literal)`
representation (`EscapedPicture`/`parse_picture_escapes`), then
threading `forced_literal` by index through the existing digit-
placeholder/decimal-point/comma-grouping checks -- which previously only
ever looked at the character value itself -- rather than teaching each
of those checks to separately look one character back for an escaping
`\`. `REQ-0220`'s own `;` section-splitting became escape-aware in the
same change, copying an escaped pair through untouched instead of
testing its second character as a separator, since a picture's own
escapes are not expanded until after it has already been split into
sections. Manually verified every escape shape via `wfc --eval` before
writing formal tests; the self-referential `\\` case needed the C++ unit
test itself as the real verification instrument, after ad hoc shell/
Windows-argv backslash quoting made a reliable command-line repro
impractical to construct by hand. Also updated `REQ-0218`/`REQ-0220`'s
Scope sections to point at this requirement instead of listing the
escape character as excluded.

**Format custom picture quoted literal text (increment #105,
`REQ-0222`):** picked from `REQ-0221`'s own Scope, which had just
excluded quote-delimited literal text as "VB6's other mechanism" for the
same job the `\` escape character does. Every character between a pair
of `"` in a custom picture is now a plain literal, and the quote
delimiters themselves never appear in the output. Implemented by
extending `REQ-0221`'s `parse_picture_escapes` to also track a
quote-toggle state, marking every character inside a `"..."` run
`forced_literal` the same way an escaped `\X` pair already is -- reusing
the exact same downstream rendering logic with no further changes
needed there -- and by making the `;` section-splitting loop
quote-aware the same way it is already backslash-aware, so a `;` inside
a quoted run does not split the picture into `REQ-0220` sections. An
unterminated quoted run (no closing `"` before the section ends) makes
the rest of that section literal too, mirroring the escape character's
own "trailing lone `\`" simplification. Manually verified quoted text
before and after a placeholder, a quoted `;` not splitting a picture,
and quoted text combined with a real unquoted section separator, via
`wfc --eval` before writing formal tests -- since a `Style` argument is
itself a VB6 string literal, every test needed VB6's own doubled-quote
(`""`) escaping at the language level before this requirement's own
picture-level `"..."` delimiting ever saw the resulting string value.
Also updated `REQ-0221`'s Scope to point at this requirement instead of
listing quoted text as excluded.

**Format custom picture `%` scaling (increment #106, `REQ-0223`):**
picked from `REQ-0218`'s original Scope, which had deferred `%` and
`E+`/`E-` together as one bullet. An unescaped, unquoted `%` anywhere in
a custom picture section now scales the value by 100 before any digit
is matched against a placeholder, the same scaling the named `Percent`
style already applies -- checked after `REQ-0220`'s section split and
`REQ-0221`/`REQ-0222`'s escape/quote expansion, so an escaped `\%` or a
quoted `"%"` is a plain literal with no scaling, and a two-section
picture can scale one section without scaling the other. The `%`
character needed no new *rendering* logic at all: it was never one of
this format's own special characters, so it already passed through as
an ordinary literal once the underlying value was scaled -- the whole
feature reduces to "detect an unescaped `%`'s presence in the section,
then scale." Manually verified scaling for a positive and negative
value, an escaped/quoted `%` suppressing scaling, and per-section
scaling in a two-section picture, via `wfc --eval` before writing formal
tests. Updated `REQ-0218`'s Scope to note `%` is no longer excluded,
leaving only `E+`/`E-` (a materially larger feature) under that bullet.

**IsMissing for an omitted Optional Variant argument (increment #107,
`REQ-0224`):** picked from `REQ-0206`'s own Scope, which had left
`IsMissing` hardcoded `False` but explicitly documented the one case
real VB6 itself distinguishes -- an `Optional Variant` parameter with no
explicit default, omitted by the caller. `IsMissing(paramName)` now
returns `True` in exactly that case; a required parameter, a
non-`Variant` `Optional`, a *defaulted* `Variant` `Optional` (the
default counts as supplied in real VB6), and any name that is not a
parameter of the current procedure at all still answer the pre-existing
constant `False`. Implementing this needed a dedicated argument-parsing
branch, apart from the generic `parse_expression()`-per-argument loop
every other intrinsic function shares: an omitted Optional argument is
synthesized into a real value (the default) before that generic loop
would ever see it, indistinguishable from a caller-supplied one, so the
only way to answer correctly is to check the argument's own *name*
directly, which only a bare-identifier parse (not expression
evaluation) preserves. `invoke_definition` now records each call's own
omitted no-default Variant Optional parameter names into a new
`Scope::missing_parameter_names` set at the exact point the omission is
still known. Two of this evaluator's own three pre-existing `IsMissing`
tests needed updating -- not a regression, but because they asserted
the *old* stubbed behavior for syntax (`IsMissing(7)`, a non-identifier
argument) that real `IsMissing` was never actually valid for; one
arity test's own diagnostic correctly changed from `WFC0072` to
`WFC0011` for the same reason (`IsMissing(1, 2)`'s first argument was
never a valid parameter reference, independent of how many followed
it). Manually verified the omitted/supplied case, the non-Variant-
Optional-stays-False case, and -- reasoning from `REQ-0206`'s own prior
documented nuance rather than assuming -- the defaulted-Variant-
Optional-still-False case, via `wfc --eval` before writing formal
tests.

**Unconditional Do...Loop (increment #108, `REQ-0225`):** with the
work log's own aggregated backlog now fully worked down to items each
individually larger or riskier than the increments just completed,
dispatched an `Explore` subagent (owner: "keep working") to re-scan
every `requirements/language/*.md` Scope section directly, rather than
picking the next item from memory or re-reading all 85 files inline.
It ranked unconditional `Do...Loop` (from `REQ-0148`'s own Scope) first
of five candidates: touches one function, no design-invariant
conflicts (unlike `ReDim`-as-declaration), no exhaustive-dispatch blast
radius (unlike `CVErr`). Its report was independently spot-checked
against `src/evaluator.cpp` (confirmed the exact `WFC0040` branch and
line numbers in `parse_posttest_do_statement`) before acting on it. A
bare `Do`/`Loop` with no `While`/`Until` on either line now repeats
forever, ended only by `Exit Do`/`Exit For` (`REQ-0150`, reused
unchanged) -- the same thing `Do While True` already let a caller
express, just without writing a condition. Implemented by accepting a
statement end directly after `Loop` as a third case alongside `While`/
`Until`, skipping condition-parsing entirely and relying only on
`Exit Do`/`Exit For` (or `enclosing_execution` being false, for the
existing dead-branch dry-run convention every other loop form already
follows). This correctly turned previously-invalid syntax into valid,
non-terminating-unless-exited syntax -- surfaced immediately by the
full local suite hanging on its very first post-change `ctest` run,
since one pre-existing `WFC0040` test (`Do\nPrint "no"\nLoop`,
asserting the *old* "missing condition is always an error" behavior)
became a genuine infinite loop with no `Exit Do` in its body; killed
the hung `wfc_frontend_tests.exe`/`ctest.exe` processes via
`taskkill //F //T`, then corrected that one test to use genuinely
malformed trailing text after `Loop` instead. Manually verified `Exit
Do` from inside an `If` block, `Exit Do` reached from inside a nested
`For` loop (crossing that loop's own boundary to end the outer
unconditional one), and dead-branch dry-run parsing (no output, no
hang) -- each run under a `timeout` guard, given the fresh memory of
the hang just found -- before writing formal tests.

**Option Base (increment #109, `REQ-0226`):** the second pick from the
same `Explore` subagent's five-candidate report (owner: "keep
working"). `Option Base 1` now changes a *bound-less* array dimension's
lower bound from `0` to `1` -- `Dim` and `ReDim` alike, every dimension
of a multi-dimensional array -- following the exact module-level
placement rules `Option Explicit`/`Option Compare` (`REQ-0158`/
`REQ-0159`) already established. An explicit `<lower> To <upper>`
dimension stays unaffected, and a `ParamArray`'s array stays `0`-based
regardless -- a real, documented VB6 exception the subagent's own
report had flagged as needing confirmation rather than assumption,
verified here by direct manual test. Implemented as a single
evaluator-wide `option_base_one_` bool, mirroring `option_explicit_`/
`option_compare_text_`'s own existing single-flag convention (this
evaluator has no per-module/per-class `Option` scoping for the other
two directives either), consulted at exactly the two sites that
defaulted a bound-less dimension's lower bound to a hardcoded `0` --
`Dim`'s own array-bound parser and `ReDim`'s separate, textually-
identical bound-parsing loop -- both found via `grep` for the literal
`dimension_lower = 0;` assignment rather than re-deriving the parser's
structure from scratch. New diagnostics `WFC0152`/`WFC0153` follow the
exact pattern `Option Explicit`/`Option Compare` already use for
"duplicate directive"/"invalid value" failures. Manually verified
`Option Base 1` for a 1-D array, every dimension of a multi-dimensional
array, `ReDim`'s own shorthand, an explicit-bound dimension staying
unaffected, a `ParamArray` staying `0`-based, the no-`Option Base`
default, and both new diagnostics, via `wfc --eval` before writing
formal tests.

**Inferred-type constants (increment #110, `REQ-0227`):** the third and
final pick from the same `Explore` subagent's five-candidate report.
`Const x = 5`, with neither a type-declaration character nor an `As
Type` clause, now infers its type from the initializer expression
instead of requiring an explicit declared type to check against -- the
real-VB6 asymmetry with a bare `Dim x`, which instead already defaults
to `Variant` in this evaluator: an untyped `Const` never becomes
`Variant`, it takes on whatever concrete type its initializer naturally
produces. `Const x = 5` is `Long` here, not real VB6's own `Integer` --
a pre-existing, evaluator-wide simplification (`REQ-0199`'s own Scope
already discloses that unsuffixed integer literals are `Long`
throughout this evaluator), so an inferred-type constant simply
inherits it rather than introducing a new, Const-specific divergence.
Implemented by reordering `parse_constant_declaration`: with no
explicit type present, the initializer is parsed *first* and its own
value's type becomes the constant's type directly, skipping the
type-mismatch check entirely (there is nothing declared to check it
against). The subagent's own report had correctly flagged this
candidate's real risk -- which type an inferred integer constant should
get -- resolved here by deferring to this evaluator's own pre-existing,
already-disclosed convention rather than inventing a new one. Manually
verified an inferred `Long`/`String`/`Boolean`/`Double` constant, a
derived inferred constant, and the existing `WFC0064` diagnostic still
applying, via `wfc --eval` before writing formal tests.

**Class-typed and generic Object parameters (increment #111,
`REQ-0228`):** with the aggregated backlog and the first `Explore`
subagent's five candidates now exhausted, dispatched a second `Explore`
subagent (owner: "keep working") to specifically re-scope the remaining
class-features backlog cluster into concrete sub-items rather than
attempting one of its bigger-sounding bullets directly. It investigated
three candidates -- a class-typed method/property parameter, `Static`
arrays/object references, and a `Private` class declaration -- and
ranked the first small and tractable, correctly flagging a real lifetime
risk in its own report before any code was written; the other two were
either blocked by a deeper pre-existing lifetime gap (`Static`) or not a
real gap at all given this evaluator's flat, single-module class model
(`Private` class). `As Object`/`As SomeClassName` is now accepted for
*any* Sub/Function/Property parameter, generalizing what was previously
accepted only for `Property Set`'s own single value parameter --
reusing the exact `parse_scalar_object_or_class_type` resolver a
class-typed field/return type already uses, and threading a specific
class's name into the callee's own frame so a `Set param = ...` inside
the body is class-checked by the existing, unmodified `Set` machinery.
Manual testing surfaced two real bugs the subagent's report had
anticipated in outline: ByRef write-back copied a parameter's final
value over the caller's variable with a plain assignment, never
checking whether the caller's own prior value was an `ObjectInstance`
about to lose its last reference -- unreachable before this requirement,
immediately reachable once object-typed parameters existed; fixed by
calling `terminate_if_last_reference` (the same helper `Set` already
calls) immediately before each ByRef write-back, verified safe for the
ordinary unchanged-parameter case too. And `zero_value_for_index` had no
case for `Nothing`'s own index, so an omitted `Optional Object` argument
bound `Boolean False` instead of `Nothing`; fixed by adding the missing
case. Manual testing also hit an unrelated, pre-existing bug along the
way -- `If obj Is Nothing Then ... Else obj.Member End If` fails during
dry-run parsing of the untaken `Else` branch even with a plain `Dim o As
Object` and no parameters involved at all -- confirmed unrelated, then
flagged via `spawn_task` for separate follow-up rather than fixed here,
to keep this increment's own diff focused.

**Member access on Nothing inside a not-taken branch (increment #112,
`REQ-0229`):** the follow-up bug just flagged above (owner: "keep
working"). Rather than leave it as a `spawn_task` suggestion for a
separately-dispatched session, dismissed that task and fixed it here
directly, since this session already had the exact repro and root cause
loaded -- cheaper than a fresh session re-deriving both from scratch.
`If obj Is Nothing Then ... Else <access obj.Member> End If` raised
`WFC0106` ("Invalid use of Nothing") even for the untaken `Else` branch,
because that check ran unconditional of `execute_`, unlike every other
value-dependent runtime check this evaluator already suppresses during a
dead branch's own dry-run parse (arithmetic overflow, division by zero).
Fixed at all three dotted-member-access sites that can reach a `Nothing`
base (a field/`Property Get` read, a `Property Let`/field write, a
`Property Set` write): when `!execute_`, each still parses through any
required `(args)`/`= expression` syntactically, so the source text and
shape stay validated, but returns or assigns a placeholder instead of
resolving a member against a class that, for a genuinely `Nothing` base,
does not exist at all. Deliberately left `CStr(Nothing)`/`Nothing`
concatenation's own `WFC0106` untouched: that is a different,
always-invalid-regardless-of-live-state type mismatch, closer in kind to
the pre-existing `WFC0020` "wrong operand type" check (which already
fires in dead branches too), not a value-dependent runtime check the
dry-run convention is meant to suppress. Manually verified all three
access shapes plus a method call with arguments, via `wfc --eval` before
writing formal tests.

**Comma-separated Next variables, attempted and reverted (increment
#113, would-be `REQ-0230`):** `Next j, i`, closing two nested `For`/`For
Each` loops with one `Next` (owner: "keep working"). The closing-name
handoff worked once debugged: a `pending_next_variable_` member,
checked at the top of `parse_for_body` before scanning for a literal
`Next`, correctly carries the outer loop's own name across the call
boundary -- but only once also reset on every retry of the *same*
loop's own iteration, since this interpreter re-parses a loop's body
from scratch on every iteration and an earlier iteration's own pending
name would otherwise still be sitting there, unconsumed, at the top of
the next iteration's fresh parse, looking exactly like a real match and
mismatching against that loop's own identifier. What could not be
fixed within this attempt's own scope: this evaluator requires every
statement to be followed by its own terminator, consumed by a generic
"parse a statement, then consume its terminator" wrapper used
pervasively across the module body, every procedure/class-method body,
and every `If`/`Do`/`While`/`For`/`Select Case` body. `Next j, i` is one
physical line with one terminator but conceptually closes *two*
statements (the inner loop, then the outer one); the inner loop's own
wrapper correctly consumes that one terminator, leaving the outer
loop's own *separate* required terminator-consumption step with
nothing left once real code follows on the next line. This was masked
at first by a misleading successful repro: placing `Next j, i` as the
very last line of the program "worked" only because
`consume_statement_end()`'s own `at_end()` check tolerates being
satisfied twice, not because the underlying design was sound -- the
very next test, with an ordinary `Print` statement following, failed
with `WFC0004`. Fixing this properly needs a "did a nested statement
already consume the shared terminator" signal threaded through every
one of those wrapper call sites, not a `For`-loop-local change. Rather
than ship a partial version artificially restricted to exactly two
loops (which would still break as soon as real code followed the
`Next` line, since the shared-terminator problem is not a comma-count
problem), reverted the entire uncommitted `src/evaluator.cpp` change
via `git checkout --` (after confirming via `git status`/`git diff
--stat` that nothing else was uncommitted) and recorded the specific
obstacle in the backlog instead, so a future attempt starts from the
real difficulty rather than rediscovering it.

**Static Variant object-lifetime fixes (increment #114, `REQ-0230`,
reusing the number the reverted attempt above never actually
committed):** found while the second `Explore` subagent, back in
increment #111, scoped a possible `Static` arrays/object-typed-locals
extension and flagged a real lifetime gap in the *existing* `Static
Variant` + `Set` combination -- independent of that unimplemented
extension, and already reachable today. Two bugs, both fixed: the
end-of-call copy-back that persists a `Static`'s final value
overwrote the previous persistent value with a plain assignment, never
checking whether it was the last reference to an `ObjectInstance` (the
same class of bug `REQ-0228` fixed for a ByRef parameter's own
write-back) -- fixed by calling `terminate_if_last_reference` on the
old persistent value first. And program-end cleanup only ever drained
the module scope, never any procedure's or class member's own
persistent `statics` storage, so an instance reachable only through a
`Static Variant` at program end never ran `Class_Terminate` at all --
fixed by draining every procedure's and every class method's/property
accessor's own `statics` alongside the module scope. Manually traced
and verified the exact termination ordering across two calls to the
same `Sub` before writing formal tests confirming the same sequence.

**Static arrays and class-typed/Object locals (increment #115,
`REQ-0231`):** the remaining half of the class-cluster backlog item
`REQ-0230` had just closed the lifetime-bug half of (owner: "work on
static first"). `Static o As Object`/`As SomeClassName` now accepts an
object reference the same way a class-typed field/return/parameter
already does, reusing `parse_scalar_object_or_class_type` unchanged;
`REQ-0230`'s two `Class_Terminate` lifetime fixes apply automatically
with no further code, since both operate on any object-holding
`Static`, not specifically a `Variant`-declared one -- verified
directly with a `Static` class-typed local persisting an instance
cumulatively across three calls. `Static name(<bounds>) As Type` adds a
fixed-size array, 1-D or multi-dimensional, persisted and copied
element-for-element the same way a scalar `Static`'s single value
already was; unlike `Dim`, `Static` has no dynamic array form at all
(real VB6 requires fixed bounds at declaration time, with no `ReDim`
ever possible for one), so `Static arr()` reports `WFC0149` instead of
being treated as an unallocated dynamic array, and a `Variant`/`Object`
element type is rejected the same way. Extracted the bound-list
grammar out of `Dim`'s own fixed-size array parsing into a new shared
`parse_fixed_array_bounds` helper -- ran the full suite with *no test
changes at all* immediately after the extraction to confirm it was
purely mechanical before adding `Static`'s own use of it, rather than
debugging both changes together. Manually verified a 1-D `Static`
array's elements persisting across three calls, a multi-dimensional
`Static` array, and both new `WFC0149` rejections, via `wfc --eval`
before writing formal tests. Caught and fixed a numbering slip before
committing: the code comments initially, mistakenly, referenced
`REQ-0232` (skipping `REQ-0231` entirely); renamed to the correct
sequential number after a `grep` sweep confirmed nothing had been
committed under the wrong one yet, the same check used when the
reverted `Next`-comma-list attempt's own number was reused earlier.

**`Implements` interfaces (increment #116, `REQ-0233`):** owner: "do
interfaces," picked directly from the class-cluster backlog ahead of
class inheritance, COM interop, lazy `As New`, and `Class_Terminate`
field-cascading. An interface is just an ordinary `--class` class in
real VB6, so `Implements InterfaceName` simply records another class
name on `ClassDef`; a new shared `class_satisfies(actual, declared)`
helper widens both `Set`'s and `REQ-0228`'s parameter-binding
class-match checks to accept any implementing class instead of only an
exact identity match. Dispatch is a static-declaration-driven rewrite:
`parse_member_access_after_dot` gained an optional `via_interface_class`
parameter, populated at the three call sites able to cheaply obtain a
base expression's declared interface type -- `parse_primary()` (via a
speculative identifier lookahead that captures and restores `offset_`),
the `Call obj.Method(args)` path, and the bare `obj.Method` path (`Call
Me.Method` deliberately left unthreaded, since `Me` has no declared
interface type of its own) -- rewriting the member name to
`InterfaceName_MemberName` whenever the live instance's own class
actually implements the named interface. Manual testing found a real
bug: a `Private Sub IShape_Draw()` was unreachable even through a
correctly `IShape`-typed reference, because the pre-existing per-class
`Private`-visibility check had no notion of a call being sanctioned by
interface dispatch; fixed with a `bypass_for_interface_dispatch`
parameter threaded from `parse_member_access_after_dot`'s own
`dispatched_via_interface` local, verified correctly scoped (direct
access to the same literal name, and a non-interface reference's bare
unprefixed name, both still correctly fail). Manually verified
polymorphic dispatch across two implementing classes, interface-typed
parameter dispatch, the `Private`-bypass fix and its two negative
cases, a `Set` class mismatch, an unknown interface name, and
`Property Get` dispatch through a second interface on a
two-interface class, via `wfc --eval` before writing formal tests.

**`Class_Terminate` field-cascading (increment #117, `REQ-0234`):**
owner: "keep working," the next class-cluster backlog item after
`REQ-0233`. `terminate_if_last_reference` previously only ever checked
the single top-level `ObjectInstance` it was called on; when that
instance was actually destroyed, any of its own fields holding the last
reference to another instance lost that reference through plain C++
`shared_ptr` refcounting with no `Class_Terminate` notification ever
propagating -- a gap `REQ-0204`'s own Scope had explicitly disclosed as
not implemented. Fixed by having `terminate_if_last_reference` call
`drain_scope_instances(instance->fields)` immediately after handling the
instance's own `Class_Terminate`, reusing that existing helper
completely unchanged against `InstanceData::fields` (itself a `Scope`,
identical in shape to a call frame's locals) instead of writing new
field-walking logic -- the cascade therefore recurses automatically to
arbitrary depth, since a cascaded field's own termination is just
another call to the same function. The cascade runs regardless of
whether the dying instance's own class declares `Class_Terminate`, and
correctly does not terminate a field's instance early when a second
reference to it survives elsewhere. Manually verified a single-level
cascade both with and without the outer class declaring its own
`Class_Terminate`, and a two-level cascade (`Top` -> `Middle` ->
`Deepest`) terminating in the correct outer-to-inner order, via
`wfc --eval` before writing formal tests. While constructing these
manual repros, found and flagged via `spawn_task` (not fixed here, to
keep this increment focused) a separate, unrelated pre-existing gap: a
chained two-level field write (`o.i.tag = 9`) reports `WFC0108` instead
of writing through to `tag`; every repro/test here uses an intermediate
local variable at each level instead. Updated `REQ-0204`'s Scope (the
original exclusion) and `REQ-0205`/`REQ-0228`'s own Scope sections
(which had each reaffirmed it) to point at this requirement.

**Remaining next increments:**

- comma-separated `Next` variables (`Next j, i` closing two nested `For`/
  `For Each` loops with one `Next`) -- attempted and reverted (owner:
  "keep working"): the closing-name matching itself works (a
  `pending_next_variable_` member, checked at the top of `parse_for_body`
  before scanning for a literal `Next`, correctly hands the outer loop's
  own name across the call boundary, once also reset on each iteration
  retry of the *same* loop to stop it leaking into that loop's own next
  re-parse). What breaks is more fundamental: this evaluator requires
  every statement to be followed by its own terminator, consumed by a
  generic "parse a statement, then consume its terminator" wrapper used
  pervasively (module body, procedure/class-method bodies, every
  If/Do/While/For/Select Case body). `Next j, i` is textually one line
  with one trailing terminator, but conceptually closes *two* statements
  (the inner loop, then the outer one) -- the inner loop's own wrapper
  correctly consumes that one terminator, leaving the outer loop's own
  *separate* required terminator-consumption step with nothing left to
  consume once real code follows on the next line (confirmed by a
  minimal repro: `Next j, i` at the very end of the program "worked" only
  because `consume_statement_end()`'s own `at_end()` check tolerates
  being satisfied twice, masking the bug until a real statement follows).
  Fixing this correctly needs a "did a nested statement already consume
  the shared terminator" signal threaded through every one of those
  wrapper call sites, not just the `For`-loop-specific code touched so
  far -- a materially larger, cross-cutting change than the rest of this
  feature, reverted rather than shipped incomplete;
- class inheritance, `CreateObject`/
  `GetObject`/COM interop, array-of-class/array-of-`Object` elements
  (a class-typed/`Object`-typed *scalar* parameter is now covered,
  `REQ-0228`, but not an array-typed one -- see `REQ-0214`/`REQ-0215`'s
  own Scope), lazy `As New` auto-instantiation, and a `Private` *class*
  declaration itself (not a real
  gap: this evaluator has no per-module visibility concept at all for a
  whole class to be private *from*, the same situation module-level
  `Private`/`Public` is already in) -- (`Static` arrays and `Static`
  object-typed locals, also originally listed here, were closed by
  `REQ-0231`, after `REQ-0230` fixed the two `Class_Terminate`-lifetime
  bugs a `Static Variant` holding an object reference already had;
  interfaces (`Implements`), also originally listed here, were closed by
  `REQ-0233`; `Class_Terminate` cascading to an instance only reachable
  through the terminated one's own fields, also originally listed here,
  was closed by `REQ-0234`) -- all deliberately excluded from `REQ-0203`/`REQ-0204`/`REQ-0205`/
  `REQ-0206`'s scope, alongside the same exclusions `REQ-0202`
  already lists for module-level procedures (`IsMissing` for an omitted
  `Optional Variant` argument, also originally listed here, was closed
  by `REQ-0224`);
- `ByVal`/`Optional` array parameters (neither valid in real VB6, so
  neither is implemented -- not a gap, a correct rejection); a
  class-typed array-typed parameter (`REQ-0211`/`REQ-0215` cover a
  concrete scalar, `Variant`, or generic `Object` element, not a specific
  class); a `Variant`/`Object`/class-typed/multi-dimensional array
  *return* type for a `Function` (`REQ-0216` covers a fixed-scalar,
  single-dimension return type only); `Property Get` returning an array
  at all; and indexing a function's array result directly (`Foo()(i)`,
  the caller must assign the result to a variable first) -- all
  deliberately excluded from `REQ-0214`/`REQ-0215`/`REQ-0216`'s scope;
- `ReDim` as an implicit first declaration (real VB6 allows `ReDim x(5)`
  with no prior `Dim` at procedure scope) -- this evaluator's `ReDim`
  always requires a prior `Dim identifier()` (`REQ-0207`'s Scope);
- late binding and `CVErr`/error-value Variants (`IsError` stays
  hardcoded `False`; `IsMissing` now returns a real answer for an
  omitted no-default `Optional Variant` argument, `REQ-0224` -- every
  other case, and `IsError`/`CVErr` entirely, remain excluded). `CVErr`
  in particular was scoped out deliberately, not merely deferred: it
  would add a wholly new `Value` alternative that every one of this
  evaluator's ~27 unchecked `std::get<T>` call sites (arithmetic,
  comparison, rendering) would need auditing against, a materially
  larger and riskier change than its one-line backlog description
  suggested;
- `Format`'s `E+`/`E-` scientific notation combined with a custom
  picture, a fourth (text) custom-picture section, and the `Date`/`Time`
  named styles listed in `REQ-0193`'s Scope (`Currency`, the core
  custom-picture tokens, up to three `;`-separated numeric sections,
  both literal-text mechanisms -- `\` escape and `"..."` quoting -- and
  `%` scaling are now implemented, `Currency` under a disclosed
  fixed-locale rendering);
- a parenthesis-free call *with* one or more arguments, in any form
  (`Foo 5, 6`, `obj.Method 5, 6`) -- deliberately excluded from
  `REQ-0213`/`REQ-0217`'s "zero arguments only" scope, to avoid the
  classic ambiguity between that form and other statement/expression
  shapes. Every argument-taking call still needs parentheses.

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

## MP-0002 status after the corpus-hardening series (increment #172 onward)

Measured on the x64 debug build: 244 CTest tests pass (unit suite, CLI and
project-fixture integration tests, and 95 corpus programs under
`tests/corpus/`). GitHub Actions x86, x64 and arm64 pass on the latest commits
(the arm64 preset uses the `Visual Studio 18 2026` generator; x86/x64 stay on
`windows-2022` / VS 2022).

**Implemented against the MP-0002 principal-work list:** procedures (named and
omitted arguments, ParamArray, ByRef array elements), module state, expressions
(VB precedence, Boolean as -1/0), statements (single-line loops, nested
single-line `If`), arrays, UDTs (fixed strings, `Len`, `Get`/`Put`), variants,
strings, `Format` (named, custom, scientific, date and string pictures), errors
(`On Error`/`Resume`/`Err`/`GoTo`/`GoSub`; handlers run in the failing
statement's context so `Resume Next` works inside loops), conditional
compilation, `DefType`, the core value types, file I/O (sequential, Binary,
Random), `Date`/`Time`, financial functions, `.vbp`/`.bas`/`.cls` loading, and
class modules (`Implements`, default members, `Event`/`RaiseEvent`/`WithEvents`,
`As New` fields, program-wide class Enums, `Collection`,
`CreateObject("Scripting.Dictionary")`), and, since then: built-in `Scripting.Dictionary`/
`FileSystemObject`/`VBScript.RegExp` (late or early bound) on a native hash-indexed store,
numbered lines and `Erl`, single-line and `Static` procedures, `NewEnum`, `VBA.`-qualified
calls, nested array indexing, ByRef through fields, Variant arithmetic/comparison rules,
Byte/Integer result types, month-name dates, `Format` half-up rounding, `Declare` timing
and `MessageBox` emulation, `file:line:col` diagnostics, and VB6 `Print` number spacing
for project runs (REQ-0281, REQ-0282).

**Known remaining gaps** (each documented in its requirement's Scope): `Get`/`Put` of
Variants and dynamic-array descriptors; class inheritance; other
`CreateObject`/`GetObject` ProgIDs and COM interop (MP-0003); visual items in `.vbp`
files (MP-0004); real `SendKeys`; asynchronous `Shell`;
`LenB`/`LeftB`/`MidB`/`AscB` still report WFC's stored UTF-8 bytes (REQ-0177), not VB6's UTF-16 byte layout.
The deterministic VB6 reference-probe corpus beyond the existing spot probes has
not been extended in this series.

**Resolved during the series:** unsuffixed small integer literals are now `Integer`
(REQ-0280) and over-`Long` literals are `Double`.

**Owner decisions still open for the MP-0002 exit gate** (recommendations drafted in
`planning/legacy-feature-dispositions.md`, each awaiting acceptance): the six legacy
features listed in `planning/compatibility-profile-1.0.md` under "Legacy
Features Requiring Explicit Disposition" (DDE, intrinsic `Data`/DAO, OLE1,
ActiveX Documents, PropertyPage hosting, WinHelp) remain "Proposed; not yet
accepted" -- these are scope decisions for the maintainer, not something this
series changed.

**Next responsible party:** the maintainer or a subsequent assistant session,
continuing the corpus-driven hardening under MP-0002.

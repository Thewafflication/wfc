# Core VB Language Requirements

**Content type:** Project requirement index

**Status:** Proposed

This requirement set defines source-language behavior implemented during
MP-0002. Each increment states its exact compatibility boundary so passing a
narrow corpus cannot be mistaken for complete VB6 language support.

## Requirements

- [REQ-0140 — Core expression execution subset](req-0140-core-expression-subset.md)
  defines the first typed expression grammar, values, operators, results, and
  stable failures accepted by the `Print` evaluator.
- [REQ-0141 — Module statement and variable subset](req-0141-module-statements-and-variables.md)
  defines sequential execution, typed module variables, assignments, comments,
  and multi-line output for the second MP-0002 increment.
- [REQ-0142 — Boolean and comparison expression subset](req-0142-boolean-and-comparison-expressions.md)
  defines Boolean values, comparisons, logical operators, precedence, and
  strict type failures for the third MP-0002 increment.
- [REQ-0143 — Single-line conditional statement subset](req-0143-single-line-conditional-statements.md)
  defines Boolean-gated `If ... Then ... Else` execution and branch diagnostics
  for the fourth MP-0002 increment.
- [REQ-0144 — Block conditional statement subset](req-0144-block-conditional-statements.md)
  defines multiline `If ... Then` / `Else` / `End If` execution and structural
  diagnostics for the fifth MP-0002 increment.
- [REQ-0145 — Block ElseIf branch subset](req-0145-block-elseif-branches.md)
  defines ordered `ElseIf` selection, first-match execution, and branch
  diagnostics for the sixth MP-0002 increment.
- [REQ-0146 — While...Wend loop subset](req-0146-while-wend-loops.md)
  defines pre-test repetition, nested loop execution, and loop diagnostics for
  the seventh MP-0002 increment.
- [REQ-0147 — Pre-test Do loop subset](req-0147-pretest-do-loops.md)
  defines `Do While` and `Do Until` execution and diagnostics for the eighth
  MP-0002 increment.
- [REQ-0148 — Post-test Do loop subset](req-0148-posttest-do-loops.md)
  defines `Loop While` and `Loop Until` execution and diagnostics for the ninth
  MP-0002 increment.
- [REQ-0149 — Rem comment subset](req-0149-rem-comments.md) defines
  case-insensitive, statement-level `Rem` comments for the tenth MP-0002
  increment.
- [REQ-0150 — Exit Do subset](req-0150-exit-do.md) defines structured,
  innermost-loop termination for the eleventh MP-0002 increment.
- [REQ-0151 — For...Next loop subset](req-0151-for-next-loops.md) defines
  declared-`Long` counter iteration for the twelfth MP-0002 increment.
- [REQ-0152 — Exit For subset](req-0152-exit-for.md) defines structured,
  innermost counter-loop termination for the thirteenth MP-0002 increment.
- [REQ-0153 — Select Case equality subset](req-0153-select-case.md) defines
  ordered, same-type equality branches for the fourteenth MP-0002 increment.
- [REQ-0154 — Select Case value-list subset](req-0154-select-case-value-lists.md)
  defines comma-separated equality alternatives for the fifteenth MP-0002
  increment.
- [REQ-0155 — Select Case range subset](req-0155-select-case-ranges.md) defines
  inclusive numeric and string ranges for the sixteenth MP-0002 increment.
- [REQ-0156 — Select Case Is subset](req-0156-select-case-is.md) defines
  relational branch clauses for the seventeenth MP-0002 increment.
- [REQ-0157 — Typed constant subset](req-0157-typed-constants.md) defines
  immutable `Long`, `String`, and `Boolean` values for the eighteenth MP-0002
  increment.
- [REQ-0158 — Option Explicit subset](req-0158-option-explicit.md) defines
  strict module directive placement for the nineteenth MP-0002 increment.
- [REQ-0159 — Option Compare subset](req-0159-option-compare.md) defines binary
  and ASCII case-insensitive string comparison for the twentieth MP-0002
  increment.
- [REQ-0160 — Len function subset](req-0160-len-function.md) defines the first
  callable VBA string function for the twenty-first MP-0002 increment.
- [REQ-0161 — String case function subset](req-0161-string-case-functions.md)
  defines ASCII `LCase` and `UCase` calls for the twenty-second MP-0002
  increment.
- [REQ-0162 — String trim function subset](req-0162-string-trim-functions.md)
  defines `Trim`, `LTrim`, and `RTrim` calls for the twenty-third MP-0002
  increment.
- [REQ-0163 — String slice function subset](req-0163-string-slice-functions.md)
  defines `Left` and `Right` calls and multi-argument call parsing for the
  twenty-fourth MP-0002 increment.
- [REQ-0164 — Mid function subset](req-0164-mid-function.md) defines one-based
  substring extraction and optional call arguments for the twenty-fifth
  MP-0002 increment.
- [REQ-0165 — Character code function subset](req-0165-character-code-functions.md)
  defines ASCII `Asc` and `Chr` conversion for the twenty-sixth MP-0002
  increment.
- [REQ-0166 — StrReverse function subset](req-0166-strreverse-function.md)
  defines deterministic byte reversal for the twenty-seventh MP-0002
  increment.
- [REQ-0167 — VbCompareMethod expression subset](req-0167-vb-compare-method.md)
  defines the three named comparison constants and explicit `InStr`/`StrComp`
  comparison selection for the twenty-eighth MP-0002 increment.
- [REQ-0168 — Replace options subset](req-0168-replace-options.md) defines
  one-based start, bounded replacement counts, and explicit comparison for the
  twenty-ninth MP-0002 increment.
- [REQ-0169 — InStrRev function subset](req-0169-instrrev-function.md) defines
  reverse string search and its optional start/comparison arguments for the
  thirtieth MP-0002 increment.
- [REQ-0170 — Val Long subset](req-0170-val-long.md) defines decimal-prefix
  parsing into the current `Long` value model for the thirty-first MP-0002
  increment.
- [REQ-0171 — Integer math function subset](req-0171-integer-math-functions.md)
  defines `Abs` and `Sgn` for `Long` values in the thirty-second MP-0002
  increment.
- [REQ-0172 — CStr function subset](req-0172-cstr-function.md) defines String
  conversion for every current value type in the thirty-third MP-0002
  increment.
- [REQ-0173 — CLng function subset](req-0173-clng-function.md) defines Long,
  Boolean, and strict decimal String conversion for the thirty-fourth MP-0002
  increment.
- [REQ-0174 — CBool function subset](req-0174-cbool-function.md) defines Boolean,
  Long, and strict Boolean/decimal String conversion for the thirty-fifth
  MP-0002 increment.
- [REQ-0175 — CByte function subset](req-0175-cbyte-function.md) defines bounded
  Long-to-Byte conversion for the thirty-sixth MP-0002 increment.
- [REQ-0176 — Scalar Information function subset](req-0176-scalar-information-functions.md)
  defines the Information-module behavior representable by the current scalar
  value model, including `RGB` color packing.
- [REQ-0177 — Byte-string function subset](req-0177-byte-string-functions.md)
  defines `LenB`, `AscB`, and `ChrB` over MP-0002's current byte-string model.
- [REQ-0178 — StrConv case-conversion subset](req-0178-strconv-function.md)
  defines the representable ASCII case-mapping modes and their `VbStrConv`
  constants.
- [REQ-0179 — Source-visible VBA constant enumerations](req-0179-source-visible-vba-constants.md)
  defines the complete source-visible `Vb*` and general constant table.
- [REQ-0180 — Round function](req-0180-round-function.md) defines nearest-even
  rounding for the current numeric model and bounded decimal-place selection.
- [REQ-0181 — Double numeric value type](req-0181-double-numeric-type.md)
  introduces fractional literals, promoted arithmetic, comparison, rendering,
  and conversion behavior.
- [REQ-0182 — CDbl and CSng conversions](req-0182-double-conversions.md)
  defines current-scalar conversion to `Double` and Single-precision narrowing.
- [REQ-0183 — Floating-point math functions](req-0183-floating-point-math-functions.md)
  defines `Sqr`, `Sin`, `Cos`, `Tan`, `Atn`, `Exp`, and `Log` over the current
  numeric model.
- [REQ-0184 — Double declarations and introspection](req-0184-double-declarations.md)
  extends typed variables, constants, `TypeName`, and `VarType` to `Double`.
- [REQ-0185 — Representable numeric literal suffixes](req-0185-numeric-literal-suffixes.md)
  defines `#` (`Double`) and `&` (`Long`) numeric literal forms.
- [REQ-0186 — Representable identifier type-declaration characters](req-0186-identifier-type-characters.md)
  defines `#`, `&`, and `$` declarations and type assertions.
- [REQ-0187 — Int and Fix over Long and Double](req-0187-int-fix-double.md)
  defines floor/truncation semantics for the current numeric model.
- [REQ-0188 — Str over Long and Double](req-0188-str-double.md) defines
  sign-space conversion for both current numeric value types.
- [REQ-0189 — Hex and Oct over Long and Double](req-0189-radix-double.md)
  defines banker's-rounded radix conversion for both numeric value types.
- [REQ-0190 — CVar scalar identity conversion](req-0190-cvar-scalar.md)
  defines Variant identity conversion for every currently representable scalar
  value type.
- [REQ-0191 — MacID four-byte conversion](req-0191-macid-function.md) defines
  source-visible Macintosh four-character-code packing in the current byte
  string and `Long` model.
- [REQ-0192 — Error message function subset](req-0192-error-function.md)
  defines selected stable VBA run-time error descriptions and the documented
  fallback for undefined valid numbers.
- [REQ-0193 — Format named numeric styles](req-0193-format-named-styles.md)
  defines the eight named `Format`/`Format$` styles over the current `Long`,
  `Double`, and `Boolean` value model.
- [REQ-0194 — Rnd and Randomize](req-0194-rnd-randomize.md) defines the
  reference-verified default `Rnd` sequence and a WFC-owned deterministic
  reseed contract for `Randomize`/`Rnd(negative)`.
- [REQ-0195 — Single numeric value type](req-0195-single-numeric-type.md)
  defines the distinct `Single` type: the `!` literal suffix and identifier
  character, `Dim`/`Const As Single`, `Long`/`Single`/`Double` arithmetic
  promotion, and extending every existing `Long`/`Double` intrinsic to accept
  `Single`.
- [REQ-0196 — Currency numeric value type](req-0196-currency-numeric-type.md)
  defines the distinct `Currency` type: a scaled-int64 fixed-point
  representation with exact (not floating-point) arithmetic, the `@` literal
  suffix and identifier character, `Dim`/`Const As Currency`, four-way
  `Long`/`Currency`/`Single`/`Double` arithmetic promotion, `CCur`, and
  extending every existing numeric intrinsic to accept `Currency`.
- [REQ-0197 — Scalar Variant value type](req-0197-variant-scalar-type.md)
  defines the scalar `Variant`: `Dim x As Variant`/bare `Dim x` freely
  retyping across every representable value type, the `Null`/`Empty`
  literals and states, `IsNull`/`IsEmpty`, and three-valued (Kleene) logic
  propagation of `Null` through every operator and condition check.
- [REQ-0198 — Decimal numeric value type](req-0198-decimal-numeric-type.md)
  defines the distinct `Decimal` type reachable only through `Variant`: an
  exact 96-bit-mantissa, variable-scale (0-28) representation matching COM's
  `DECIMAL`, `CDec`, and exact `+`/`-`/`*`/`/` arithmetic with banker's
  rounding.
- [REQ-0199 — Integer numeric value type](req-0199-integer-numeric-type.md)
  defines the distinct 16-bit `Integer` type: the `%` literal suffix and
  identifier character, `Dim`/`Const As Integer`, checked narrowing from
  `Long`/`Single`/`Currency`/`Double`, `Integer` < `Long` < `Currency` <
  `Single` < `Decimal` < `Double` arithmetic promotion, `CInt` now returning
  a genuine `Integer`, and extending every existing numeric intrinsic to
  accept `Integer`.
- [REQ-0200 — Minimal object-reference stub](req-0200-minimal-object-reference.md)
  defines `Nothing` as a distinct value state, `Dim x As Object`, the `Set`
  statement (the only legal way to assign an object reference), the `Is`
  operator for object identity, and `IsObject`/`TypeName`/`VarType`
  returning real results for it.
- [REQ-0201 — Fixed-size one-dimensional arrays](req-0201-fixed-size-arrays.md)
  defines `Dim arr(n)`/`Dim arr(lo To hi) As Type` arrays over the
  `Integer`/`Long`/`Double`/`Single`/`Currency`/`String`/`Boolean` element
  types, indexed read/write, `LBound`/`UBound`, and `IsArray`/`TypeName`/
  `VarType` returning real results for arrays.
- [REQ-0202 — User-defined Sub/Function procedures](req-0202-user-defined-procedures.md)
  defines module-level `Sub`/`Function` declarations callable from anywhere
  in the module (a pre-scan pass, forward reference, recursion), `ByVal`/
  `ByRef` parameters, per-call local variable scope, `Function` return via
  self-name assignment, `Exit Sub`/`Exit Function`, and the `Call`
  statement.
- [REQ-0203 — Class modules foundation](req-0203-class-modules.md) defines
  class modules supplied as separate sources alongside the standard module
  (`wfc --class <Name> <source>`), covering field declarations, `Sub`/
  `Function` methods, `Property Get`/`Let`/`Set` accessors, `New`/
  `Dim x As New ClassName`/`Dim x As ClassName`, `.` member access,
  unqualified sibling-member calls (implicit `Me`), and `Is`/`TypeName`/
  `VarType`/`IsObject` for a live instance.
- [REQ-0204 — The Me keyword and Class_Initialize/Class_Terminate](req-0204-me-and-class-lifecycle.md)
  defines the explicit `Me` keyword and the `Class_Initialize`/
  `Class_Terminate` lifecycle hooks REQ-0203 deferred: `Class_Initialize`
  runs automatically at `New`, after fields are initialized; `Class_Terminate`
  runs when a variable is found to hold an instance's last reference (at
  `Set`, at the end of a call for that call's own locals, and at the end of
  a successful program for module-level variables) — a documented
  approximation of VB6's reference-counted lifetime, not full COM-style
  refcounting.
- [REQ-0205 — Class-module refinements](req-0205-class-module-refinements.md)
  defines three further REQ-0203 follow-ups: an unqualified sibling
  `Property Let`/`Set` write (local variables/parameters correctly shadow a
  same-named property); class-typed and `As Object` fields and `Function`/
  `Property Get` return types, with forward references between classes
  resolved regardless of `--class` order (a two-pass `scan_classes`); and
  indexed `Property Get`/`Let`/`Set` accessors (`obj.Name(args)`, alongside
  the existing zero-parameter form).
- [REQ-0206 — Optional/ParamArray/Static parameters and Public/Private
  visibility](req-0206-procedure-modifiers.md) extends both module-level
  procedures and class members with `Optional [= default]` parameters,
  `ParamArray` (collecting trailing call arguments into a fresh array),
  `Static` locals (persisting across separate calls to the same
  procedure), and `Public`/`Private` visibility (a bare `Dim` class field
  is now implicitly `Private`; a `Private` member is accessible from any
  method of the *same class*, on any instance, and nowhere else).
- [REQ-0207 — Dynamic arrays: ReDim and ReDim Preserve](req-0207-dynamic-arrays.md)
  defines `Dim identifier()` (an unallocated dynamic array) and the
  `ReDim [Preserve] identifier(<bound>)` statement that allocates or
  reallocates it, extending `REQ-0201`'s fixed-size array foundation.

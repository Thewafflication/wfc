# REQ-0211 — Array-typed Sub/Function parameters

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0201, REQ-0202

## Requirement

The evaluator shall recognize `name() As Type` as a `Sub`/`Function`
parameter, declaring an array parameter of the given element type
(`Integer`, `Long`, `Double`, `Single`, `Currency`, `String`, or
`Boolean` — the same fixed list `Dim`'s array form accepts). The
parameter's own declaration does not fix a bound or a dimension count:
any array of the matching element type binds to it, one dimension or
several.

- An array parameter is always effectively `ByRef`: the caller's argument
  must be a bare identifier naming an already-declared array variable of
  the matching element type (`WFC0016` on an element-type mismatch,
  `WFC0149` if the argument is not a variable at all). Every mutation the
  callee makes through the parameter — writing an element, or `ReDim`/
  `ReDim Preserve`ing it (`REQ-0207`) to a new size — is written back to
  the caller's variable when the call returns, reusing the same ByRef
  write-back mechanism an ordinary bare-identifier `ByRef` scalar argument
  already uses (`REQ-0202`).
- `LBound`, `UBound`, indexed read/write, `Erase` (`REQ-0208`), and
  `For Each` (`REQ-0209`) all work on an array parameter exactly as they
  do on any other array variable, including a multi-dimensional one
  (`REQ-0210`): the parameter's own declaration is dimension-count-
  agnostic, so the bound array's actual dimension count applies inside
  the callee.
- Extends both module-level `Sub`/`Function` declarations and class
  methods/properties (`REQ-0203`), since both already share one parameter
  parser.

## Diagnostics

`WFC0149` reports: an explicit `ByVal` on an array parameter (rejected —
an array is a reference-like value in real VB6, and this evaluator has no
other way to alias the caller's array); `Optional` on an array parameter
(rejected — not implemented); a malformed array-parameter declaration
(missing the element type, or a non-empty `()`, mirroring `ParamArray`'s
own `WFC0141`); and a call argument that is not a variable at all.
`WFC0016` reports an element-type mismatch between the argument and the
parameter. `WFC0072` reports the ordinary wrong-arity case (an array
parameter is always required, matching every parameter that is not itself
`Optional` or the trailing `ParamArray`).

## Scope

This requirement adds only a plain array parameter's binding and
write-back. It does not add:

- `ByVal` array parameters — not valid in real VB6 either;
- `Optional` array parameters, or an array-typed `ParamArray` element
  (`ParamArray`'s own elements are always scalar; an array of arrays does
  not exist in this evaluator);
- passing a `Variant`/`Object`-element array was originally excluded
  here (neither existed yet); `REQ-0215` later added both. A literal/
  expression array remains impossible to pass (no array-valued expression
  exists other than a bare variable reference);
- returning an array from a `Function` was originally excluded here;
  `REQ-0216` later added a fixed-scalar array return type.

## Verification

- `tests/evaluator_tests.cpp` covers summing and doubling a `Long` array
  through a `Function` parameter (verifying the doubled values write back
  to the caller), a `ReDim Preserve` inside a `Sub` parameter growing the
  caller's array, a multi-dimensional array binding to a plain `nums() As
  Long` parameter, an element-type mismatch (`WFC0016`), `ByVal`/`Optional`
  rejection (`WFC0149`), and a class method's own array parameter.
- `TC-MP0002-array-parameter-cli` verifies the sum-and-double round trip
  through `wfc --eval`.

## Traceability

This requirement extends `REQ-0202`'s user-defined-procedure parameter
model and `REQ-0201`/`REQ-0207`/`REQ-0210`'s array foundation with the
array-typed parameter form both left as future work, reusing `REQ-0202`'s
existing bare-identifier ByRef write-back mechanism unchanged.

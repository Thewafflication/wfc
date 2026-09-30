# REQ-0243 — `Collection`, parenthesis-free calls with arguments, and related fixes

## Statement

- Built-in `Collection`: `Add Item[, Key[, Before[, After]]]` (Before/After
  accepted, ignored — items append), `Count`, `Item(index-or-key)` (keys are
  case-insensitive), `Remove index-or-key`, and `For Each ... In collection`
  over scalars or objects. Duplicate keys raise error 457, a bad key 5, a
  bad index 9. Implemented as VB source registered on demand (program text
  mentions `Collection` and defines no class of that name) using an
  internal `WfcCollectionNode` class and a `WfcItems` iteration hook.
- Parenthesis-free calls with arguments: `Name a, b` for a module
  procedure or sibling method and `obj.Method a, b` (closes the exclusion
  from `REQ-0213`/`REQ-0217`).
- `Set a.b.c = x` chained `Set`, `Set field = x` for Variant fields,
  `Set arr(i) = x` for Variant arrays, and `Err.Raise` omitted arguments.
- Fixes: `Exit Function`/`Exit Sub` directly inside a `Do` body no longer
  loops forever; a procedure called from inside a block may declare locals;
  value-dependent `Set` and `ReDim` bound errors are no longer raised for
  not-taken branches.

## Scope

No default member (`c(1)` / `c!key`), `Before`/`After` positioning,
`Collection` as an `Implements` target, or `Keys`. Bare-argument calls need
the callee to be a known procedure/method (not an arbitrary expression).

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-collection-cli`.

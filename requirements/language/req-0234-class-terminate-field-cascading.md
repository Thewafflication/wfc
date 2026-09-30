# REQ-0234 — `Class_Terminate` field-cascading

**Status:** Implemented
**Milestone:** MP-0002 — Core VB/VBA Language Execution
**Depends on:** REQ-0200, REQ-0203, REQ-0228, REQ-0230

## Requirement

The evaluator shall cascade `Class_Terminate` into an instance's own
fields when that instance itself becomes the last reference and
terminates, matching real VB6's reference-counted object lifetime: when
an instance is freed, every field reference it held is released along
with it, and a field that was *itself* only reachable through the dying
instance must have its own `Class_Terminate` run in turn (and so on,
recursively, for that field's own fields).

`terminate_if_last_reference` (the shared helper already responsible for
running `Class_Terminate` at every well-defined lifetime point — `Set`'s
overwrite, a ByRef parameter's write-back, a `Static`'s copy-back, and a
call frame's/the module's own scope drain) now, immediately after
handling the instance itself, drains the instance's own field scope the
same way a call frame's locals are drained at the end of a call —
reusing `drain_scope_instances` unchanged against `InstanceData::fields`
instead of a `Scope`'s locals. This cascade:

- runs whether or not the dying instance's own class declares
  `Class_Terminate` — an instance's fields go out of scope along with it
  regardless of whether it has a termination hook of its own;
- correctly does **not** terminate a field's instance early when another
  reference to it survives elsewhere (e.g. a local variable also holding
  `Set` to the same instance) — the field is still released (cleared),
  but the shared instance itself is only terminated once its own
  `use_count()` genuinely reaches zero;
- recurses to arbitrary depth through nested class-typed fields (a field
  whose own class has a field, whose own class has a field, ...),
  terminating from the outermost dying instance inward.

## Diagnostics

No new diagnostic codes. The cascade only ever runs `Class_Terminate`
bodies that were already valid to run on their own (the pre-existing
`REQ-0200` mechanism); any error such a body itself raises is propagated
as `terminate_if_last_reference`'s own failure, exactly as it already was
for the top-level instance.

## Scope

This requirement closes the field-cascading exclusion `REQ-0204`'s own
Scope section originally listed (and `REQ-0205`/`REQ-0228` each
reaffirmed in turn). It does not add:

- cascading through a class-typed **array** field or element — class-typed
  arrays remain out of scope entirely (`REQ-0214`/`REQ-0215`'s own Scope,
  reaffirmed by `REQ-0231`'s own Scope for `Static` arrays); there is
  nothing to cascade into until that exists;
- cascading through a `Variant` field that happens to hold an object via
  `Set` — `drain_scope_instances` (reused unchanged here) already handles
  this uniformly, since it checks `std::holds_alternative<ObjectInstance>`
  regardless of whether the field's static type is `Object`, a specific
  class, or `Variant`; this is not new to this requirement, it is simply
  inherited for free from the existing helper;
- breaking a **reference cycle** (instance A's field holds instance B,
  B's field holds A) — real VB6 itself leaks in this case (the classic
  COM circular-reference problem, since a plain refcount never reaches
  zero for either), and this evaluator's `shared_ptr`-based model
  reproduces the identical outcome: neither instance's `use_count()` ever
  reaches 1 while the cycle holds, so neither's `Class_Terminate` ever
  runs, matching real VB6 rather than adding a cycle collector VB6 itself
  never had.

## Verification

- `tests/evaluator_tests.cpp` covers: a field holding the last reference
  to another instance, cascading correctly after the outer instance's own
  `Class_Terminate` runs; the same cascade when the outer class declares
  no `Class_Terminate` of its own at all; a two-level cascade (`Top` ->
  `Middle` -> `Deepest`) terminating in the correct outer-to-inner order;
  and (via the second test) a field's instance correctly *not*
  terminating early while a second local-variable reference to it still
  exists, only doing so once that second reference is also released.
- `TC-MP0002-class-terminate-cascade-cli` verifies the two-level cascade
  through `wfc --class ... --eval`.
- Manually verified all of the above via `wfc --eval` before writing
  formal tests, including confirming the cascade fires correctly
  regardless of unordered_map iteration order between two aliasing local
  variables in the same scope (relying on `drain_scope_instances`'s
  existing "clear as it goes" behavior, unchanged here, to keep the
  use_count accounting correct regardless of which aliasing variable is
  visited first).
- While reproducing this via `wfc --eval`, found a separate, unrelated
  pre-existing gap: a chained two-level field write (`o.i.tag = 9`, where
  `o.i` is itself an object-typed field) reports `WFC0108` ("object
  assignment requires Set") instead of writing through to `tag` — a
  single-level write (`n.tag = 9`) and a single-level `Set` both work
  correctly; only writing a *scalar* field two or more dots deep fails.
  Deliberately not fixed here (a chained-assignment parsing gap, not a
  lifetime bug); every test here uses an intermediate local variable at
  each level instead, and the gap was flagged separately via `spawn_task`
  for its own follow-up.

## Reference

- [Microsoft VBA `Class_Terminate` event reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/terminate-event)

## Traceability

This requirement closes the "`Class_Terminate` cascading to an instance
only reachable through the terminated one's own fields" item
`REQ-0204`'s own Scope section originally listed among this evaluator's
class-lifecycle exclusions (reaffirmed by `REQ-0205`/`REQ-0228`'s own
Scope sections in turn), picked directly by the owner ("keep working")
from the same class-cluster backlog `REQ-0233` (`Implements` interfaces)
had just been picked from, leaving class inheritance, `CreateObject`/
`GetObject`/COM interop, and lazy `As New` auto-instantiation as the
cluster's remaining items.

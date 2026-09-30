# REQ-0235 — Chained field write

## Statement

`o.i.tag = expression`, where `o.i` is an object-typed (class, `Object`, or
`Variant`-held) field, writes `expression` to `tag` on the instance `o.i`
references, at any depth (`a.b.c.d = 1`).

## Scope

- Each intermediate link must be a field holding an object reference; a
  non-object field reports `WFC0136`, a `Nothing` field reports `WFC0106`.
- Intermediate links that are `Property Get` results are not covered; only
  fields are (use an intermediate local variable).

## Verification

- `tests/evaluator_tests.cpp` chained-write case.
- `TC-MP0002-chained-field-write-cli`.

## Traceability

Closes the gap noted in `REQ-0234`'s Verification section, where
`o.i.tag = 9` reported `WFC0108` instead of writing through.

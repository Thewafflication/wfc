# REQ-0279 — `Scripting.Dictionary` and default-member assignment

## Statement

- `CreateObject("Scripting.Dictionary")` returns a built-in Dictionary
  (provided as VB source, like `Collection`) with `Add`, `Item` (get / let /
  set, default member), `Exists`, `Remove`, `RemoveAll`, `Count`, `Keys`,
  `Items`, `Key`, `CompareMode`, and `For Each` over the keys. Keys compare
  case-sensitively unless `CompareMode = 1`; reading a missing key adds an
  `Empty` entry, as the real object does. `TypeName` is `"Dictionary"`.
- `obj(args) = value` and `Set obj(args) = ref` assign through the class's
  default member (Property Let / Set).
- Other ProgIDs still raise error 429.

## Verification

Corpus `37-dictionary`, `38-default-let`.

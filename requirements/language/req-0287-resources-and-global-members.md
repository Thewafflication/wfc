# REQ-0287 — Resource files and headless `Global` members

**Content type:** Project requirement

**Status:** Accepted

## Statement

- **Resource files.** A project's `ResFile32=` entry names a compiled `.res`
  file. `LoadResString(id)` returns string `id` from the string tables (16
  strings per block, block number `id \ 16 + 1`, text stored as UTF-16);
  `LoadResData(id, type)` returns the resource's bytes as a `Byte()` array,
  where `id` and `type` are numbers or names (case-insensitive). A missing
  resource or file raises error 326 ("Resource with identifier '`id`' not
  found"). `LoadResPicture` needs picture objects and belongs to MP-0005.
- **`Forms`** (`REQ-0067`) is the empty forms collection until forms exist
  (MP-0004): `Count` is 0, `For Each` visits nothing and `Forms(i)` raises 9.
- **Not provided without forms or pictures:** `Load`, `Unload`,
  `LoadPicture`, `SavePicture`, `Printer`, `Printers`, `Licenses`.

## Verification

`TC-MP0002-corpus-145-resources` (with a small `.res` file) and
`-146-forms-collection`.

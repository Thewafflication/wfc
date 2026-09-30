# REQ-0245 — Sequential file I/O, `Print` lists, directory helpers, `Environ`

## Statement

- `Open path For Input|Output|Append [Access ...] [Shared|Lock ...] As [#]n
  [Len = n]`, `Close [#n, ...]` (no arguments closes all), `FreeFile`,
  `EOF(n)`, `LOF(n)`, `Loc(n)`, `Input(count, #n)`.
- `Print #n, items`, `Write #n, items` (strings quoted, `#TRUE#`/`#FALSE#`,
  dates `#yyyy-mm-dd hh:nn:ss#`), `Input #n, vars...`, `Line Input #n, var`.
  Files are written with CRLF line ends and read tolerant of LF/CRLF.
- `Print` (console and file) takes an item list separated by `;` (join) or
  `,` (next 14-column zone), `Spc(n)`, `Tab(n)`, and a trailing separator
  suppresses the newline. Numbers keep this evaluator's existing no-padding
  rendering (real VB6 adds a leading sign space).
- `Kill`, `MkDir`, `RmDir`, `FileCopy`, `Dir(pattern)`/`Dir()`, `FileLen`,
  `CurDir`, `Environ(name)`.
- Failures raise catchable errors: 52 bad file number, 53 file not found,
  54 bad file mode, 55 already open, 62 input past end, 75/76 path errors.

## Scope

Binary/Random modes, `Get`/`Put`/`Seek`, `Name ... As ...`, `ChDir`,
`GetAttr`/`SetAttr`, `Lock`/`Unlock`, `Environ(n)` by index, and `Dir` with
attribute arguments are not implemented. `Dir` patterns use `*`/`?` via the
`Like` matcher, ASCII case-insensitive.

## Verification

`tests/evaluator_tests.cpp` (temp-directory round trip);
`TC-MP0002-file-io-cli` (Print lists).

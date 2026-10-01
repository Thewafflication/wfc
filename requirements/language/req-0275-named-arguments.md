# REQ-0275 — Named arguments

## Statement

- A call to a user `Sub`/`Function`/method may pass `name:=value` arguments in
  any order after (optional) positional ones: `Show times:=2, msg:="yo"`,
  `obj.Add(1, c:=5)`. Skipped Optional parameters take their defaults.
- An unknown name raises error 448; naming a parameter twice (or after a
  positional argument for it) and a positional argument after a named one
  are rejected.

## Scope

Built-in functions (`MsgBox prompt:=...`) do not take named arguments yet.

## Verification

Corpus `31-named-arguments`.

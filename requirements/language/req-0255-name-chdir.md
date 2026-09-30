# REQ-0255 — `Name ... As ...` and `ChDir`

## Statement

`Name old As new` renames a file or directory (errors 53 file not found,
58 file already exists, 75 access error); `ChDir path` changes the process
working directory (error 76). A variable called `Name` still works: the
statement form is recognised only when `Name` is followed by an expression,
not `=`, `(`, `.` or `,`.

## Verification

`tests/evaluator_tests.cpp`; `TC-MP0002-name-chdir-cli`.

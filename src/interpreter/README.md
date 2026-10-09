# Interpreter Sources

**Content type:** Source directory guide

The VB evaluator's internals, split out of the former single
`src/evaluator.cpp`. Everything here is in namespace `wfc::detail` and is not
part of the public API (`include/wfc/`). `src/evaluator.cpp` keeps the source
preprocessing (line continuations, bracketed identifiers, conditional
compilation) and the public `wfc::evaluate_program` entry points.

## Headers

Each header includes the one before it, so including a later header brings in
everything earlier.

| Header | Contents |
| --- | --- |
| `vb_numeric.hpp` | Integer type aliases, `Currency`, `Decimal`, and their arithmetic |
| `vb_value.hpp` | `Empty`/`Null`/`Nothing`, arrays, object instances, and the `Value` variant |
| `vb_date.hpp` | Date serials, calendar arithmetic, and date helpers |
| `vb_text.hpp` | Three-valued logic, UTF-16 string helpers, identifiers, VBA constants |
| `builtin_class_sources.hpp` | VB source of the built-in classes (`Collection`, `Scripting.Dictionary`, `FileSystemObject`, `RegExp`, ...) |
| `interpreter.hpp` | `Scope`, procedure and class definitions, and the `Interpreter` class declaration |

## `Interpreter` member definitions

| Source | Contents |
| --- | --- |
| `core.cpp` | Program setup, scopes, lexing primitives, type keywords, runtime errors |
| `declarations.cpp` | Module, class, procedure, UDT, `Dim`/`Static`/`Const`/`ReDim` declarations |
| `statements.cpp` | Statement dispatch, control flow, `On Error`, `Print`, `Exit` |
| `assignment.cpp` | Assignment, `Set`, property `Let`/`Set`, array elements, `Mid` statement |
| `calls.cpp` | Procedure calls, arguments, class instances, member access |
| `expressions.cpp` | Expression parsing, literals, conversions, comparison, arithmetic |
| `format.cpp` | `Format` and numeric rendering |
| `files.cpp` | File I/O statements and functions |
| `builtin_functions.cpp` | The intrinsic VBA function library (`parse_function_call_impl`) |
| `builtin_misc.cpp` | Miscellaneous and date/time functions, `Rnd`/`Randomize` |

A new member function is declared in `interpreter.hpp` and defined in the
source whose theme fits it. New diagnostic codes are registered in
[`requirements/language/diagnostics.md`](../../requirements/language/diagnostics.md).

# REQ-0314 — The comparison operators compare as Visual Basic 6 does

**Content type:** Project requirement

**Status:** Implemented

**Milestone:** MP-0010 — Distribution, Libraries and Command Line

**Source:** The owner's instruction that WFC behave exactly as Visual Basic 6
does, measured with `evidence/reference/operator-probe`.

**Depends on:** REQ-0142, REQ-0313

## Requirement

The operators `=`, `<>`, `<`, `<=`, `>` and `>=` shall answer for two operands
of any kinds as Visual Basic 6.00.8176 does. The rules were measured, not read
from the documentation: 4,695 comparisons of 38 kinds of operand were run under
Visual Basic 6 and the answers are in
`evidence/reference/operator-probe/vb6-reference.txt`.

An operand is **typed** (a variable or expression that is not a Variant) or a
**Variant** (a Variant variable, `Null` or `Empty`). The result is a Boolean,
or `Null` when an operand is Null (in any operator).

- **Empty** is 0 beside a number, a Boolean or a Date, and `""` beside a String;
  two Empties are equal.
- **Two typed numbers** (Boolean and Date count as numbers) compare by the wider
  type: a Currency beside any number compares as Currency (both rounded to four
  places), a Single beside a Double or a whole number compares as Single, True
  is 255 beside a Byte, and otherwise the values compare exactly (so `Integer 5`
  and `Double 5.4` are not equal: nothing is rounded to the integer).
- **Two typed Strings** compare as text, by `Option Compare` of the module.
- **A typed String beside a typed number, Boolean or Date** is converted to that
  type; a String that is not a number (a Boolean also reads `True` and `False`)
  is error 13. `"5" = 5` is True; `"abc" = 5` is error 13.
- **A Variant String beside a typed number, Boolean or Date** is converted to
  that type when it can be, and is the greater of the two (no error) when it
  cannot.
- **A typed String beside a Variant** that holds a number, Boolean or Date
  compares as text (the Variant as `CStr` gives it).
- **Two Variants** put every number, Boolean and Date before every String
  (`Variant 5 = Variant "5"` is False, `<` is True); two Strings compare as
  text; two numbers as above.

An error found while comparing is a trappable run-time error; code that does
not run compares nothing.

## Verification

**Method:** Test

`TC-MP0002-operator-probe-interpret` and `TC-MP0002-operator-probe-native`
generate the probe (`scripts/New-OperatorProbe.ps1`), run it under WFC and
compare all 4,695 lines with `vb6-reference.txt`. `tests/evaluator_tests.cpp`
and corpus 82 and 113 cover the cases of earlier requirements that changed
(`"1" = 1` is True; a String that is not a number beside a number is error 13
only when both are typed).

## Relationships

- **Derived from:** the owner's instruction of 2026-11-04
- **Revises:** REQ-0142 (mixed-type comparisons are no longer diagnostic
  `WFC0018`)
- **Conflicts with:** None known

## Implementation Record

`Interpreter::operator_order` and `Interpreter::compare_operands`
(`src/interpreter/select_case.cpp`). The interpreter's `parse_comparison` tells
which operand holds a Variant; native code passes that in the flags of the
`variant_binary` service and compares two operands of one type (or two whole
numbers) directly; the small runtime for Windows 2000
(`src/win2000/wfcw2k.c`, `variant_compare_vb`) has the same rules for the types
it has.

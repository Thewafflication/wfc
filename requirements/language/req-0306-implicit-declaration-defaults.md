# REQ-0306 — Implicit loop variables and default-property assignment

**Content type:** Project requirement

**Status:** Proposed

## Statement

- **Loop variables.** Without `Option Explicit`, an undeclared name used as the
  control variable of `For` or `For Each` declares itself like an assignment
  target does (`REQ-0265`): a Variant, or the type of its suffix (`n%`) or
  default type.
- **Default property assignment.** `Text1 = "x"` and `Text1(i) = "x"` assign the
  default property of a form or control: `Text` for text, combo, list and image
  combo boxes, `Caption` for labels and frames, `Value` for buttons, check and
  option boxes, scroll bars, progress bars and sliders, `Picture` for picture
  boxes and images, `Drive`, `Path` and `FileName` for the file system list
  controls.
- **Input targets.** `Input #n, v` and `Line Input #n, v` declare an undeclared
  `v` the same way.
- **Form members.** `Form2.Text1 = "x"` assigns the default property of a control
  of another form; `Left(s, n)` in form code is the string function, not the
  form's `Left` property; a bare `End` statement may appear inside an `If` block.
- **Skipped branches.** Concatenating a control in a branch that is not taken is
  not an error.
- **`LoadPicture()`.** Called without arguments it returns an empty picture,
  which clears a picture property.

## Verification

Corpus `197-implicit-loop-variables`, `198-default-property-assignment`.

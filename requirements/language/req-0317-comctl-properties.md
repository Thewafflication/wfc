# REQ-0317 — The properties of the MSComctlLib controls answer as Visual Basic 6 does

**Content type:** Project requirement

**Status:** Implemented (the differences listed in the probe README remain)

**Milestone:** MP-0010 — Distribution, Libraries and Command Line

**Source:** The owner's instruction to compare the GUI libraries with the local
VB6, measured with `evidence/reference/control-probe`.

**Depends on:** REQ-0316

## Requirement

The scalar properties of TabStrip, Toolbar, StatusBar, ProgressBar, TreeView,
ListView, ImageList, Slider and ImageCombo shall give the type, the value or the
error that MSCOMCTL.OCX gives under Visual Basic 6. 741 reads and writes were
measured; the rules are:

- An enumeration (`Appearance`, `BorderStyle`, `MousePointer`, `OLEDragMode`,
  `Style`, `Orientation`, ...) is a Long, and a value outside its members is
  error 380. `MousePointer` also takes 99.
- `Indentation`, `ImageWidth`, `ImageHeight`, `TabFixedWidth`,
  `TabFixedHeight`, `TabMinWidth`, `SmallChange`, `LargeChange` and
  `TickFrequency` refuse a negative value (380).
- Min must stay below Max (equal is 380). A ProgressBar refuses a Value outside
  the range (380); a Slider brings it inside. `ProgressBar.Min` below 0 and
  `ControlDefault` outside the range are 380. A Slider refuses `SelLength` other
  than 0 while `SelectRange` is False.

## Verification

`TC-MP0002-control-probe-interpret` runs the probe under WFC and compares it
with `vb6-reference.txt`, but for the labels in `known-differences.txt`.

# Control probe

**Content type:** Reference evidence

The properties of the MSComctlLib controls were measured under Visual Basic 6
(MSCOMCTL.OCX 6.00.8177): for each scalar property of TabStrip, Toolbar,
StatusBar, ProgressBar, TreeView, ListView, ImageList, Slider and ImageCombo
the probe reads the default, and sets a value and reads it back (Boolean, whole
numbers from -1 to 99, Singles, colours, Strings). A result is
`<TypeName>:<text>` or `err<number>`. The answers are in `vb6-reference.txt`
(741 lines); the rules that came out are REQ-0317.

`scripts/New-ControlProbe.ps1` writes the project into this folder (the
generated files are not committed). The properties come from
`../mscomctl-2.0-type-library.json`.

To measure again, compile the project with the VB6 command line as
Administrator (the IDE refuses to load an OCX otherwise) and run the exe:

```powershell
pwsh -NoProfile -File scripts/New-ControlProbe.ps1
VB6.EXE /make ControlProbe.vbp /outdir <folder>
```

Setting `ShowTips` or `HotTracking` hangs the exe of VB6 for TabStrip, Toolbar
and StatusBar; those eight lines are `= hang` in the reference. The exe was
therefore run again after each hang with the probe skipped.

## Where WFC differs

`known-differences.txt` lists the 27 labels that WFC still answers
differently; `TC-MP0002-control-probe-interpret` checks every other line.

- `ShowTips` and `HotTracking` hang the exe of VB6 (the `= hang` lines), WFC
  answers them.
- `ImageCombo` SelStart, SelLength and SelText follow the text of the combo;
  `ListView` SortKey and SortOrder are limited by the column headers.

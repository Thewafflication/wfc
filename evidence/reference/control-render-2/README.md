# MSCOMCT2 render and geometry

`frmRender2.frm` (project `ControlRender2.vbp`) puts one of each visible control
of MSCOMCT2.OCX on a form (UpDown, MonthView, DTPicker, two FlatScrollBars,
an Animation) and writes what they report to `geometry2.txt` (Left, Top, Width,
Height, and the calendar's start of week and visible days).

- `vb6.png`: the exe compiled by VB6.
- `wfc-windows-x64.png`: `wfc.exe --interpret ControlRender2.vbp --gui`.
- `wfc-winelib.png`: the Winelib build (`wfc.exe.so`) under Wine 8.0.
- `geometry-vb6.txt`, `geometry-winelib.txt`: the measurements. WFC on
  Windows x64 and the Winelib build both write exactly the lines of VB6.

What the measurements showed and WFC now does like VB6: the Week of a
MonthView is numbered by the first week of four days (7 November 2026 is week
44), `StartOfWeek` read from a form file loses the flag bits above the day,
`VisibleDays` answers the date with the time of day of now (the form takes
`Int` of it), and the colours of the calendar default to the system colours.

Still different in the pictures: VB6's MonthView draws the arrow buttons
beside the month and the "Today" line under the calendar and has a 3-D frame;
the common control that WFC hosts draws none of them. Wine draws its own
controls (blue title, different scroll bars).

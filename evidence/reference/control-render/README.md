# Control render: how the MSComctlLib controls look

One form (`frmRender.frm`, project `ControlRender.vbp`) with a Toolbar, a
TabStrip, a ProgressBar, a Slider, a TreeView, a ListView and a StatusBar,
filled in `Form_Load` with the same code under every runtime. The caption
reports the tab and panel counts the controls start with.

| image | what ran it |
|-------|-------------|
| `vb6.png` | VB6 (6.00.8176) compiled exe, run on Windows 11 |
| `wfc-windows-x64.png` | `wfc.exe --interpret --gui`, x64 release, Windows 11 |
| `wfc-wine.png` | the same `wfc.exe` under Wine 8.0 in WSL (WSLg display) |
| `compare.png` | the three side by side |

Windows shots are the window copied from the screen. The Wine shot is made by
`grab.c` (built with `wineg++`), which copies the window DC; the black bar at
its top is the caption that the window manager draws, not the form.

## Differences seen

1. **Start state (fixed).** VB6 starts the TabStrip with 1 tab and the
   StatusBar with 1 panel (`initial tabs=1 panels=1`). WFC started both empty,
   so `Tabs(1)` and `Panels(1)` failed with 35600. A control loaded from a
   form file with no persisted tabs or panels now starts with one, and
   `wfc-windows-x64.png` shows `tabs=1 panels=1`.
2. **Toolbar.** VB6 draws raised 3-D buttons; WFC draws flat text buttons.
3. **TreeView.** A TreeView loaded from a form without a Style is text only
   in VB6 (Style 0); WFC draws lines and plus/minus boxes (Style 7). This is
   one of the known differences in `../control-probe`.
4. **Look.** A VB6 exe has no manifest, so its controls are the classic
   (comctl32 v5) ones; `wfc.exe` is manifested for themed (v6) controls. The
   ProgressBar, Slider and ListView header are close; tabs and the status
   bar differ in the 3-D edges.
5. **Wine.** Wine draws its own controls: a solid blue ProgressBar (VB6 and
   Windows WFC draw segments), flat tabs and sunken frames with a thin line.
   The layout, texts, tree, list and status panels are the same as Windows.
6. **`--screenshots` under Wine (fixed).** The PNG that `wfc.exe` wrote itself
   came out blank grey under Wine, because `PrintWindow` does not render the
   window there. Under Wine the host now copies the window DC, as `grab.c`
   does.

The Wine image is `wfc.exe` (a Windows binary) run by Wine. A Winelib build,
a native Linux program built with `wineg++`, is not made yet.

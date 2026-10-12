# REQ-0297 — TabStrip and StatusBar behavior in WFC

**Content type:** Project requirement

**Status:** Accepted (implements `REQ-0010`–`REQ-0012`, `REQ-0016`, `REQ-0017` without reference probes)

## Statement

- **TabStrip selection.** The first tab added is selected. `SelectedItem` returns
  the selected `Tab` (or `Nothing` after `DeselectAll`); assigning a tab with
  `Set` selects it. Selecting a tab other than the selected one (from code, with
  `Tab.Selected = True`, or from the user) raises `BeforeClick(Cancel)`; setting
  `Cancel` keeps the selection; otherwise the tab is selected and `Click` runs.
  Selecting the selected tab raises nothing. Removing the selected tab selects
  the previous tab (or the next when it was first) without events.
- **TabStrip layout.** Tab rectangles (`Tab.Left`/`Top`/`Width`/`Height`, read-only)
  are laid out left to right in twips: a width of `TabFixedWidth` when
  `TabWidthStyle` is `tabFixed`, otherwise from the caption length (at least
  `TabMinWidth`, default 600), a height of `TabFixedHeight` (default 300), wrapping
  to a new row with `MultiRow`. `HitTest(x, y)` returns the tab under a point or
  `Nothing`. `ClientLeft` is 60, `ClientTop` the height of the tab rows plus 30,
  `ClientWidth` the width less 120 and `ClientHeight` the height less the rows
  and 90 (all read-only). These metrics approximate the OCX; they are not probed.
- **StatusBar.** `Panels.Add(Index, Key, Text, Style, Picture)`; `Panel.Left` is
  the sum of the widths of the visible panels before it (read-only). `Style`
  `sbrNormal` (0) shows the panels, `sbrSimple` (1) shows `SimpleText`. The
  scripted actions `ui <bar> panel <index|key>` and `paneldbl` raise
  `PanelClick` (and `PanelDblClick`) with the panel.
- **Scripted input.** `ui <tabstrip> select <index|key>`.
- **Constants.** The `sbr*`, `tab*`, `tbr*`, `tvw*`, `lvw*`, `cc*` and `sld*`
  enumeration names of MSComctlLib are defined.

## Verification

`TC-MP0002-corpus-189-comctl-tabstatus`.

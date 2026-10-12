# REQ-0296 — MSComctlLib collections, value objects and the native controls

**Content type:** Project requirement

**Status:** Accepted (framework increment of MP-0006; behavior requirements `REQ-0010`–`REQ-0036` follow per control)

## Statement

- **Native controls.** `Begin MSComctlLib.<Control> Name` in a form hosts a WFC
  control (`ADR-0006`): `TabStrip`, `Toolbar`, `StatusBar`, `ProgressBar`,
  `TreeView`, `ListView`, `ImageList`, `Slider`, `ImageCombo`. Their members and
  events are the generated tables of the MSComctlLib type-library evidence plus
  the container extender members (`Left`, `Top`, `Width`, `Height`, `Visible`,
  `TabIndex`, `Name`, ...). Text properties of the form file are applied.
  `As MSComctlLib.Node`, `As Node` and the other item and collection type names
  accept the corresponding objects.
- **Collections** (`Tabs`, `Panels`, `Buttons`, `ButtonMenus`, `Nodes`,
  `ListItems`, `ListSubItems`, `ColumnHeaders`, `ListImages`, `ComboItems`) are
  one-based and have `Count`, `Item` (the default member; a number or a key),
  `Add`, `Remove`, `Clear` and `For Each`. `Add` takes its parameters
  positionally or by name; an `Index` inserts before that position, a `Key` must
  be a string and unique without regard to case.
- **Errors.** A bad index raises 35600 "Index out of bounds"; an unknown key
  35601 "Element not found"; a repeated key 35602 "Key is not unique in
  collection"; an invalid picture 481; a wrong type 13.
- **Value objects** (`Tab`, `Panel`, `Button`, `ButtonMenu`, `Node`, `ListItem`,
  `ListSubItem`, `ColumnHeader`, `ListImage`, `ComboItem`) keep their identity
  while their collection lives (`Is` compares equal), have `Index` and `Key`,
  the properties of their specification and a default property. `TypeName` is
  the item's name.
- **ProgressBar and Slider.** `Min`, `Max`, `Value` raise 380 when `Min > Max`
  or `Value` is outside the range; changing `Min` or `Max` clamps `Value`. A
  slider raises `Change` when its value changes (from code or the user) and
  `Scroll` first for user drags; `GetNumTicks`, `ClearSel` and `DoClick` exist.
- **ImageList.** `ListImages.Add(Index, Key, Picture)` returns the new
  `ListImage`; the first image sets `ImageWidth`/`ImageHeight` (pixels at 96
  dpi), which are read-only afterwards (35614); `Overlay` returns the first
  image's picture.
- **Scripted input.** `ui <target> <action> [arguments]` drives user actions of a
  common control in the headless host (`ui Form1.Slider1 value 3`).

## Win32 rendering

In the Win32 host each control is the system common control of the same kind
(`SysTabControl32`, `ToolbarWindow32`, `msctls_statusbar32`, `msctls_progress32`,
`SysTreeView32`, `SysListView32`, `msctls_trackbar32`, `ComboBoxEx32`), rebuilt
from a snapshot of the model whenever the model or a property changes; the
`ImageList` has no window: the controls that name it (`ImageList`, and `Icons`/`SmallIcons` of a `ListView`, by object or by name from a form file) draw its pictures, scaled to its image size, through the image index or key of their tabs, buttons, nodes, items and combo items (the same list is rebuilt when images are added). Clicks, selections,
expansions, check boxes, column clicks, toolbar buttons, status panels, slider
drags and combo edits become the same actions as the scripted `ui` input.
`TC-MP0006-gui-comctl` shows all nine in a window (screenshot evidence).

## Known limits

The behavior of each control beyond this framework is specified by `REQ-0010`
onward and implemented in later increments; Win32 rendering is a later stage.

## Verification

`TC-MP0002-corpus-188-comctl-basic`.

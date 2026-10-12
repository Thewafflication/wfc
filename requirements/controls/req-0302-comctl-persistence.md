# REQ-0302 — Persisted MSComctlLib state in form files

**Content type:** Project requirement

**Status:** Accepted (first increment; implements the text part of `REQ-0115` for the common controls)

## Statement

- **Loaded groups.** The `BeginProperty` groups VB6 writes for these controls
  in a `.frm` are read when the form loads: `Tabs` (`Tab1`...: `Caption`, `Key`,
  `Tag`, `ToolTipText`, `ImageIndex`), `Panels` (`Panel1`...: `Text`, `Key`,
  `Style`, `Alignment`, `Bevel`, `AutoSize`, `Object.Width`, `Object.MinWidth`,
  `ToolTipText`), `Buttons` (`Button1`... with the same entries as a `Button`, and
  `ButtonMenus` of `ButtonMenu1`...: `Text`, `Key`), `Images` (`ListImage1`...:
  `Key`, `Picture` from the `.frx` file; the first image sets the list size),
  `ColumnHeader(n)` (`Text`, `Key`, `Alignment`, `Object.Width`) and `ComboItems`.
  The first tab is selected. `Object.` prefixes are dropped; hidden entries
  (`_ExtentX`, `_Version`, `NumTabs`, ...) are ignored.
- **Binary state.** An `OleObjectBlob` entry (the MSCOMCTL stream that holds
  `TreeView` nodes and `ListView` items) is not read: the control starts with empty
  collections and `WFC0293` is written to the Immediate output.
- **`Object =` lines** naming the MSComctlLib library are accepted and need no
  registered type library.

## Known limits

The MSCOMCTL binary stream format is undocumented and no sample is available, so
designer-saved `TreeView` nodes and `ListView` items are not loaded. Writing
these groups back (the form-file writer round trip of `REQ-0292`) covers the
parsed form block only.

## Verification

`TC-MP0002-corpus-194-comctl-persisted`.

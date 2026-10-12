# REQ-0298 — Toolbar behavior in WFC

**Content type:** Project requirement

**Status:** Accepted (implements `REQ-0013`–`REQ-0015` without reference probes)

## Statement

- **Buttons.** `Buttons.Add(Index, Key, Caption, Style, Image)` with the styles
  `tbrDefault` (0), `tbrCheck` (1), `tbrButtonGroup` (2), `tbrSeparator` (3),
  `tbrPlaceholder` (4) and `tbrDropdown` (5); a style outside 0–5 or a value
  outside 0–1 raises 380. `Left`, `Top`, `Width` and `Height` are read-only
  (383) and laid out in twips: separators 120 wide, other buttons the button's
  `Width`, else `ButtonWidth`, else from the caption (at least 360); the height
  is `ButtonHeight` or 330; with `Wrappable` a full row starts a new one.
- **Pressing.** A user press of an enabled button raises `ButtonClick(Button)`
  after changing its state: a check button toggles `Value`; a button group
  presses the button and releases the rest of its contiguous group.
  Separators and placeholders ignore presses. Setting `Value` of a group button
  to pressed releases the others of the group (no event).
- **Drop-down menus.** `Button.ButtonMenus` is a collection of `ButtonMenu`
  (`Text`, `Key`, `Tag`, `Enabled`, `Visible`). The arrow of an enabled
  drop-down button raises `ButtonDropDown(Button)`; choosing an enabled menu raises
  `ButtonMenuClick(ButtonMenu)`.
- **Customization.** `Customize` does nothing (no dialog); the scripted action
  `customize` raises `Change`. `SaveToolbar(Key, Subkey, Value)` records the
  visibility of each button by key for the process and `RestoreToolbar` applies
  it; neither touches the registry.
- **Scripted input.** `ui <toolbar> button|dropdown <index|key>`,
  `ui <toolbar> menu <button> <menu>` and `ui <toolbar> customize`.

## Verification

`TC-MP0002-corpus-190-comctl-toolbar`.

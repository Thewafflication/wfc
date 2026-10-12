# REQ-0300 — ListView behavior in WFC

**Content type:** Project requirement

**Status:** Accepted (implements `REQ-0024`–`REQ-0027` and `REQ-0034` without reference probes)

## Statement

- **Items and sub items.** `ListItems.Add(Index, Key, Text, Icon, SmallIcon)`.
  `ListItem.SubItems(n)` reads and writes the text of sub item *n* (0 is the
  item's own text); writing beyond the last sub item adds empty ones; reading
  beyond raises 35600. `ListItem.ListSubItems` is a collection of `ListSubItem`
  (`Text`, `Key`, `Tag`, `ReportIcon`, `ForeColor`, `Bold`, `ToolTipText`) with
  the usual `Add(Index, Key, Text, ReportIcon, ToolTipText)`.
- **Columns.** `ColumnHeaders.Add(Index, Key, Text, Width, Alignment, Icon)`;
  `Left` is the sum of the widths of the earlier columns, `Position` the index
  and `SubItemIndex` the index minus one (all read-only).
- **Views.** `View` takes `lvwIcon` (0), `lvwSmallIcon` (1), `lvwList` (2) or
  `lvwReport` (3); other values raise 380. Items are placed (read-only
  `Left`/`Top`/`Width`/`Height`, twips) in report view as rows of 240 below a
  240 header (none with `HideColumnHeaders`) as wide as the columns; in list view
  top to bottom in columns 1440 wide; in small-icon view left to right in 1440 by
  240 cells; in icon view in 1200 by 1100 cells.
- **Selection.** `SelectedItem` returns the first selected item (or `Nothing`);
  `Set` selects it. Setting `ListItem.Selected` selects exclusively unless
  `MultiSelect` is on. `GetFirstVisible` returns the first item.
- **Sorting.** `Sorted` sorts the items by `SortKey` (0: item text; *n*: sub item
  *n*) without regard to case, ascending or (`SortOrder = lvwDescending`)
  descending, and keeps them sorted as items are added; indexes follow.
- **Search and hit test.** `FindItem(String, Where, Index, Match)` searches the
  text (`lvwText`), sub item texts (`lvwSubItem`) or tags (`lvwTag`) from `Index`
  on, matching whole text or (`lvwPartial`) a prefix, ignoring case; it returns
  the item or `Nothing`. `HitTest(x, y)` returns the item whose cell contains the
  point.
- **User actions (scripted).** `ui <list> click|dblclick <item> [ctrl]` selects
  (toggling with `ctrl` under `MultiSelect`) and raises `ItemClick(Item)`,
  `Click` (and `DblClick`); `check <item>` toggles `Checked` with `Checkboxes`
  and raises `ItemCheck(Item)`; `columnclick <column>` raises
  `ColumnClick(ColumnHeader)`; `edit <text>` completes a label edit
  (`BeforeLabelEdit`, `AfterLabelEdit(Cancel, NewString)`; manual editing needs
  `StartLabelEdit`).

## Verification

`TC-MP0002-corpus-192-comctl-listview`.

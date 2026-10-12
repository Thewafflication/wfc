# REQ-0299 — TreeView behavior in WFC

**Content type:** Project requirement

**Status:** Accepted (implements `REQ-0020`–`REQ-0023` without reference probes)

## Statement

- **Hierarchy.** `Nodes.Add(Relative, Relationship, Key, Text, Image,
  SelectedImage)`: without `Relative` the node is appended at the top level;
  with it, `tvwFirst` (0) and `tvwLast` (1) place the node first or last among
  the siblings of `Relative`, `tvwNext` (2, the default) and `tvwPrevious` (3)
  next to it, `tvwChild` (4) as its last child. `Relative` is a `Node`, an index
  or a key; an unknown one raises 35601 and a relationship outside 0–4 raises
  35607. `Index` is creation order. `Remove` removes the node and its
  descendants, clearing the selection when it was inside.
- **Navigation.** `Parent`, `Child`, `Next`, `Previous`, `FirstSibling`,
  `LastSibling`, `Root` return a `Node` or `Nothing`; `Children` counts the
  children; `FullPath` joins the texts from the root with `PathSeparator`
  (default `\`). These are read-only (383).
- **Expansion and selection.** Setting `Expanded` raises `Expand` or `Collapse`
  when it changes. `EnsureVisible` expands the ancestors and returns whether it
  changed anything. `SelectedItem` returns the selected node or `Nothing`; `Set`
  selects (a `Nothing` clears). `Sorted` (control or node) sorts the siblings by
  text without regard to case, and keeps them sorted when nodes are added.
- **Queries.** `GetVisibleCount` counts the rows of visible nodes that fit the
  height (a row is 240 twips). `HitTest(x, y)` maps a point to the node whose row
  and label contain it (indent 285 twips per level unless `Indentation` is set)
  or `Nothing`.
- **Label edit.** `StartLabelEdit` begins an edit; the scripted action
  `edit <text>` completes it for the selected node: `BeforeLabelEdit(Cancel)`,
  then `AfterLabelEdit(Cancel, NewString)`; the text changes unless either sets
  `Cancel`. With `LabelEdit = tvwManual` an edit needs `StartLabelEdit` first.
- **User actions (scripted).** `ui <tree> click <node>` selects, raises
  `NodeClick(Node)` then `Click` (with `SingleSel`, expanding the node and
  collapsing the previous one); `expand` / `collapse <node>`; `check <node>`
  toggles `Checked` with `Checkboxes` and raises `NodeCheck(Node)`;
  `key down|up|home|end|left|right|space` moves the selection through the
  visible rows, expands and collapses with right and left, and toggles the check
  box with space.

## Verification

`TC-MP0002-corpus-191-comctl-treeview`.

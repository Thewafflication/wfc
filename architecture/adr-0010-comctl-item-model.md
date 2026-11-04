# ADR-0010: An Item Model for the Collections of wfc-mscomctl

**Content type:** Architecture decision record

**Status:** Proposed (written on the owner's instruction, 2026-11-04; nothing is
implemented)

**Date:** 2026-11-04

## Context

MP-0010 moves the behaviour of the controls out of the evaluator and into the
control libraries, so that the evaluator and the COM objects of a library share
one implementation. For `wfc-mscomctl2` and for the range controls of
`wfc-mscomctl` that worked as a plain move: the rules were arithmetic on a few
numbers (`updown.hpp`, `scrollbar.hpp`, `range.hpp`), and so was the layout of
the tabs and buttons (`layout.hpp`).

What is left in `lib/wfc-mscomctl/include/wfc/mscomctl/mscomctl.inc` (about
3,500 of its 4,000 lines) is of another kind. TabStrip, StatusBar, Toolbar,
TreeView, ListView, ImageList and ImageCombo are *collections of items*, and
their behaviour is written in terms of evaluator types:

- An item is a `CItem`: a `std::map<std::string, wfc::detail::Value>` of
  properties, with `sub` items (button menus, list sub items), tree links
  (`kids`, `parent`) and a weak pointer to the object the program holds for it.
- Every operation is a function of `Interpreter&` and `ControlObject&`: it
  coerces arguments with the interpreter's rules, raises events through the
  form (`BeforeClick`, `Click`, `NodeClick`, `BeforeLabelEdit`, ...), and calls
  `ComctlHooks::after_add`, `before_remove`, `changed` to fix up the selection
  and to tell the host window.
- The rules that are worth sharing (which index or key finds an item, when a key
  is not unique, what a removal does to the selection, how a tree is walked) are
  mixed into those functions.

A COM TabStrip made through `DllGetClassObject` therefore has no `Tabs`
collection at all: the member table lists it, and nothing answers it. Moving
the code as it stands would copy `Interpreter`, `Value` and `ControlObject` into
the library, which is the opposite of the goal.

## Decision

1. **Add an evaluator-independent item model to `wfc-mscomctl`.** Header only,
   no `Interpreter`, no `Value`, no Windows. The evaluator and the COM objects
   build on it, as they do on the other models.
2. **The model owns structure, not values.** Three templates, each over an item
   payload `T` that the caller defines and the model never looks into, except
   through two accessors the caller supplies (`key(const T&)` returning the Key
   as a string, and nothing else):

   - `ItemList<T>`: an ordered list with 1-based positions, `add(position,
     item)`, `remove`, `clear`, `find(index_or_key)`, `key_in_use(key, except)`.
   - `ItemTree<T>`: an `ItemList` of roots with children, the five relationships
     of `Nodes.Add` (`tvwFirst`, `tvwLast`, `tvwNext`, `tvwPrevious`,
     `tvwChild`), parent, depth, `Path`/`FullPath` from the keys, expand and
     collapse bookkeeping, and the sort of a level.
   - `Selection`: the selection rules as small value types, one per control
     (`SingleSelection` for tabs, `MultiSelection` for list items and nodes with
     `MultiSelect`).

   Properties stay in `T`. In the evaluator `T` is today's `CItem`; in a COM
   object it is a `std::map<std::string, VARIANT>`. Each side converts and
   coerces its own values, as it does for the range controls.
3. **Operations return a status and a list of notifications; they raise
   nothing.** An operation returns `Status` (`ok`, `element_not_found`,
   `index_out_of_bounds`, `key_not_unique`, `type_mismatch`, `full`,
   `bound_image_list`, ...) and the library defines the Visual Basic numbers
   (35600, 35601, 35602, 35609, 35615 and the rest of the table in
   `mscomctl.inc`) next to the enum. The caller maps a status to its own
   failure type. An operation that has a visible effect also returns
   `Notification` values (`added`, `removed`, `selection_changed`,
   `order_changed`, `renamed`), which the caller turns into `ComctlHooks::changed`
   for the host window and into events.
4. **Cancellable events are two-step.** An operation that raises a *Before*
   event (`TabStrip.BeforeClick`, the label-edit events, `BeforeLabelEdit`)
   is split into `begin_x`, which checks the status and returns a `Pending`
   token, and `commit(token)` or `cancel(token)`. The caller raises the event
   between the two. The model never sees the event or the program's handler, so
   a COM client and the evaluator raise events by their own means (the
   connection point for COM, `raise_event` for the evaluator).
5. **The evaluator becomes a thin adapter.** `add_to_collection`, `find_item`,
   `key_in_use`, `position_of`, `press_group`, `select_tab` and the tree and
   list helpers keep their signatures at first and call the model; the argument
   parsing and the coercion of property values (which need the interpreter) stay
   where they are. Nothing in the language surface changes.
6. **COM collections come last, and are the reason for the work.** After the
   evaluator uses the model, `class_factory.hpp` gets a `Collection` hook next
   to `Behaviour`: a class that has collections answers `Tabs`, `Panels`,
   `Buttons`, `Nodes`, `ListItems`, `ColumnHeaders`, `ListImages` and
   `ComboItems` with a late-bound collection object (`Add`, `Remove`, `Clear`,
   `Count`, `Item`, `_NewEnum`) over an `ItemList<T>`, whose items are late-bound
   objects over the property map. Selection and notifications reach the sink
   through the connection point that exists today.

## Order of work

Each step lands on its own, with the whole corpus passing and the evaluator
behaving as before. The existing corpus programs are the characterization
tests: `188-comctl-basic` to `193-comctl-imagecombo` and the GUI test
`TC-MP0006-gui-comctl`, plus the comctl cases of the differential fuzzer where
they apply. Where a rule has no corpus coverage, the step first adds a program
that records the present behaviour (the Microsoft behaviour is not probed; see
below).

1. **Keys and indexes** (`ItemList`): `find_item`, `key_in_use`, position rules
   of `Add`, the error numbers. Used by all eight kinds. Unit test `items_test`.
2. **Tabs and panels**: `SingleSelection`, the sixteen-panel limit, the bound
   ImageList rule, removal selecting the neighbour.
3. **Toolbar**: the button groups (`press_group` already uses `layout.hpp`),
   button menus as sub items, check and dropdown styles.
4. **TreeView** (`ItemTree`, 590 lines today): relationships, `Path`, expand and
   collapse, `Sort`, label editing as a two-step operation, `MultiSelect`.
5. **ListView** (473 lines): items with sub items, column headers, selection,
   sort, `FindItem`, label editing.
6. **ImageList and ImageCombo**: images by key, `Overlay`-style lookups, the
   combo's indented items.
7. **COM collections** (Decision 6) for TabStrip, StatusBar and Toolbar first
   (they have the fewest members), then the rest.

Steps 1 to 6 change no behaviour that a program can see. Step 7 adds behaviour
to the COM objects only.

## Alternatives considered

- **Move `mscomctl.inc` into the library unchanged.** It is already in the
  library's include directory, and that is the problem: it compiles only inside
  `forms.cpp` because it names `Interpreter`, `ControlObject`, `Value` and
  `FormSupport`. A COM object cannot use it.
- **Make the library depend on `wfc::detail::Value`.** It would let the code move
  as written, and tie every control library to the evaluator's headers and to
  its release schedule. The libraries are meant to be built, versioned and
  linked on their own (MP-0010).
- **A template over the value type `V` instead of the item payload `T`.**
  Closer to today's `std::map<std::string, Value>`, but the model would then
  have to know which properties are keys, positions or flags. Keeping the
  payload opaque, with the Key as the only accessor, keeps the model small;
  tree links and selection flags that the model owns live in the model's own
  node records, not in the payload.
- **Callbacks for events instead of notifications.** A callback runs the
  program's handler in the middle of a mutation, which is how today's code ends
  up with `from_host` and `bind_busy` flags to avoid re-entry. Returning
  notifications and splitting cancellable operations keeps the model's state
  consistent whenever a handler runs.

## Consequences

- The eight kinds share one implementation of keys, indexes and errors, where
  `mscomctl.inc` now repeats the rules per kind.
- A COM client can use `Tabs.Add`, `Nodes.Add` and the other collections, which
  today are listed in the member table and answered by nothing.
- The model can be tested without a form, a window or an interpreter, like the
  range and layout models (`TC-MP0010-mscomctl-*` unit tests).
- The adapter in `mscomctl.inc` stays and keeps the interpreter-specific work
  (argument coercion, wrapper objects, `Value` conversions), so `mscomctl.inc`
  shrinks by less than the number of lines the model adds. The gain is the
  shared rules and the COM collections, not fewer lines.
- It is a long piece of work, about the size of the evaluator side of MP-0006
  for the common controls. Step 4 (TreeView) is the largest and the riskiest.

## Risks and open points

- **The Microsoft behaviour is not probed.** The error numbers and messages come
  from the present implementation, written from the documentation. The model
  preserves them; a probe against the installed MSCOMCTL.OCX would be a separate
  piece of work, and the model makes it cheaper to apply.
- **Hidden state in the hooks.** `ComctlHooks::after_add` and `before_remove`
  fix up the selection, the host window and other controls (an ImageList bound
  to a TreeView, a Toolbar and its ImageList). Each step has to find all of what
  they do for its kind before moving it. The characterization programs are the
  guard.
- **Weak wrapper pointers.** A program can hold an item object across a removal.
  Today `CItem` carries the weak pointer; the model must keep item identity
  stable (an `ItemId`), so the adapter can keep the same wrapper without the
  model knowing it exists.
- **Decision for the owner.** Is `Tabs.Add` and the rest through COM in scope for
  MP-0010? If not, Decision 6 and step 7 drop out, and the model still pays for
  itself only through the shared rules. The design works either way; the
  recommendation is to keep step 7, since it is what the COM objects lack.

# REQ-0289 — Form modules and the intrinsic-control object model

**Content type:** Project requirement

**Status:** Accepted (first increment; extended through MP-0004)

## Statement

- **Form modules.** A `.vbp` `Form=` entry loads a `.frm` file as a class
  module with a predeclared default instance. The form description
  (`Begin VB.Form` ... `End`, nested controls, `BeginProperty` groups, `.frx`
  resource references) is parsed at load; a malformed file fails with
  `WFC0289`. `Startup="Form1"` shows that form when the program starts. A
  project may consist of forms only.
- **Controls as fields.** Every intrinsic control of the description is a
  public field of the form holding a control object. Controls that share a name
  and carry an `Index` form a control array (`Count`, `LBound`, `UBound`,
  `Item`, default-member indexing). Unqualified names in form code resolve to
  the form's own properties, methods and controls (implicit `Me`).
- **Properties from the type library.** A form or control accepts exactly the
  properties and methods of its type-library class; property types come from
  the generated tables (`scripts/New-UiControlTables.ps1`). Assigning converts
  to the declared type (`Type mismatch` 13, `Overflow` 6); a read-only property
  raises 383; an unknown member raises 438. Properties a `.frm` omits have the
  design-time defaults.
- **Lifecycle.** `Form_Initialize` runs when the instance is created. The form
  is loaded (windows created, `Form_Load` run) on `Load Form`, `Form.Show`, or
  the first use of the form or one of its controls. `Show` raises `Activate`
  and moves focus to the first tabbable control (`GotFocus`). `Hide` raises
  `Deactivate`. `Unload Form` raises `QueryUnload` (with `UnloadMode`) and
  `Unload` (each with a `Cancel` argument that can stop the unload), destroys
  the windows and raises `Terminate`; loading again restores the design-time
  values.
- **Events.** An event of a control runs `Control_Event`, with the control's
  `Index` first for a control array; a form event runs `Form_Event`
  (`MDIForm_Event` for an MDI form). Parameters follow the type-library event.
  Assigning `Text` or `Caption` raises `Change` when the value changes;
  assigning `Value` of a check box, option button or command button raises
  `Click`; clicking a check box toggles it and clicking an option button
  selects it and clears the others. Input to a disabled control is ignored.
- **Headless host.** Windows live in an in-memory host (`ADR-0005`). Input
  comes from the file named by `WfcUiScript=` in the `.vbp`: `click`,
  `dblclick`, `focus`, `set` (replace a text box's text), `type` (key events
  and `Change` per character), `wait <ms>` (advance timers) and `close`. When
  the script ends the remaining forms are unloaded in reverse load order.
  `DoEvents` runs the queued events.

- **More behavior.** A `Timer` with `Enabled` and a nonzero `Interval` raises
  `Timer` every interval of host time (`wait`). Option buttons with the same
  container (the form or one frame) form a group. With `KeyPreview` the form
  receives `KeyDown`/`KeyPress`/`KeyUp` first and a zero key code swallows the
  event. `Forms` lists the loaded forms (`Count`, `Item`, `For Each`).

- **List and combo boxes.** `AddItem` (optional index, kept sorted when `Sorted`),
  `RemoveItem`, `Clear`, `List(i)`, `ItemData(i)`, `ListCount`, `ListIndex`,
  `NewIndex`, `Selected(i)`, `SelCount` and `Text` work. Setting `ListIndex`
  or choosing an item (`select <control> <index>` in the input script) raises
  `Click` (and `Change` for a combo box, whose `Text` follows the selection).
  An index outside the list raises 381.

- **Scroll bars.** `Value` must lie between `Min` and `Max` (else 380) and a
  change raises `Change`. The input script's `scroll <control> <value>` drags
  to a value (`Scroll`, then `Change`) and `scrollby <control> <delta>` steps
  it (`Change` only), clamped to the range.

## Known limits (extended later in MP-0004)

`Print` inside form code still writes to the program's output instead of the
form surface; graphics behavior, menus, MDI, drag and drop,
validation, modal `Show` and the Win32 host are not yet implemented.

## Verification

`TC-MP0002-corpus-151-form-basic`, `-152-form-timer-arrays`, `-153-form-lists`, `-154-form-scrollbars` and the `wfc_ui_tests` units.

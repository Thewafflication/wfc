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

- **Menus.** `VB.Menu` entries are controls (`Caption`, `Checked`, `Enabled`,
  ...) whose `Click` runs `mnuName_Click`; a disabled menu ignores clicks.
  Labels, frames, timers, shapes, lines, images and menus never take the
  keyboard focus.
- **Modal forms.** `Form.Show vbModal` runs input until the form is hidden or
  unloaded; if the input ends first the form is closed.
- **Dialogs.** `MsgBox` and `InputBox` use the answers queued by
  `answer msgbox <result>` and `answer inputbox <text>` lines of the input
  script, falling back to the default button and default text.

- **Win32 host.** `wfc project.vbp --gui` runs the forms in real windows
  (top-level form windows, standard `BUTTON`, `EDIT`, `STATIC`, `LISTBOX`,
  `COMBOBOX` and `SCROLLBAR` controls, a menu bar, timers and a message loop);
  without `--gui`, or with an input script, the headless host is used. Closing
  a window unloads its form; the program ends when no form is left. Without a
  usable desktop `--gui` falls back to the headless host.

- **Keyboard navigation.** `press <key>` in the input script (`tab`, `enter`,
  `esc`, `space`, letters, digits, `f1`..`f12`, `shift+`, `ctrl+`, `alt+`
  prefixes) sends `KeyDown`, `KeyPress` and `KeyUp` to the focused control (or
  the form). Unless the program zeroes `KeyCode`, `Tab` and `Shift+Tab` move
  the focus along the tab order (`TabIndex`, else creation order; tab stops
  must be enabled, visible and `TabStop`), `Enter` clicks the focused button or
  else the form's `Default` button, `Esc` clicks the `Cancel` button, `Space`
  clicks a focused button, check box or option button, and `Alt+letter`
  activates the control whose caption has that `&letter` (a label or frame
  moves the focus to the next tab stop).
- **Validation.** When focus moves to a control whose `CausesValidation` is
  true, the control losing focus raises `Validate(Cancel)`; if the handler sets
  `Cancel`, the focus stays where it was.

- **Collections and dynamic controls.** `Controls` lists every control of a
  form in creation order (`Count`, `Item(index|name)`, `Controls(i)`,
  `For Each`); `Parent` and `Container` give the form and the enclosing frame;
  `ActiveControl` and `Screen.ActiveForm`/`ActiveControl` follow the focus (the
  first tab stop is active while `Activate` runs). `Load Control(i)` creates a
  new element of a control array from the lowest element's design-time values
  (invisible until shown) and `Unload Control(i)` removes it; a design-time
  element cannot be unloaded (361) and loading an existing index raises 360.
  `Dim c As Control`, `As Form` and `As TextBox` (any intrinsic control type)
  are accepted, and `TypeOf c Is TextBox` and `TypeName(c)` report the control
  class.

- **Fonts and form size.** `Control.Font` is a view of the control's font
  properties (`Name`, `Size`, `Bold`, `Italic`, `Underline`, `Strikethrough`,
  `Weight`) that `Set a.Font = b.Font` copies; a `.frm` `BeginProperty Font`
  group sets them (weight 700 and up is bold). A form omitting its size has the
  VB6 default (4800 by 3600 twips, client 4680 by 3195).
- **End.** `End` in an event handler stops the program at once: no further
  events run and no form is unloaded (no `Unload` or `Terminate` events).
- **Other methods.** `ValidateControls` validates the focused control (380 if
  cancelled), `PopupMenu` runs the scripted `answer popupmenu <item>`; `Link*`
  members raise 282 (DDE is excluded); `PrintForm`, `Arrange`, `WhatsThisMode`,
  `ShowWhatsThis` and `OLEDrag` are accepted and do nothing.

- **Screenshots.** `--screenshots <folder>` (with `--gui`) saves a PNG of each
  form when it is shown and just before it is unloaded. Controls are created in
  creation order inside their frame or picture box, and a form's size is its
  client area.

- **Real dialogs.** With `--gui`, `MsgBox` shows a Win32 message box (the VB
  button, icon, default-button and modality bits are passed through; the title is
  the argument or `App.Title`) and returns the VB button value; `InputBox` shows
  a prompt dialog and returns the typed text, or an empty string when cancelled.

- **SendKeys.** `SendKeys keys [, wait]` sends characters, `{NAME}` keys (`ENTER`,
  `TAB`, `ESC`, `BACKSPACE`/`BS`, `DELETE`/`DEL`, `INSERT`, `HOME`, `END`, `PGUP`,
  `PGDN`, arrows, `F1`-`F16`, with an optional repeat count `{LEFT 3}`), `~`
  (Enter) and the `+`, `^`, `%` modifiers (also on a parenthesized group). The
  headless host delivers the keys one at a time to whichever control has the
  focus then (so `{TAB}` moves later keys on) and typing into a text box edits
  it and raises `KeyPress` and `Change`; the Win32 host injects real input.
  A bad key name raises 5.

## MDI forms

A form whose `.frm` sets `MDIChild = True` is created inside the project's
`MDIForm`, which loads and shows with it. `MDIForm.ActiveForm` returns the child
holding focus (or the topmost shown child, else `Nothing`). `Arrange` takes
`vbCascade` (0), `vbTileHorizontal` (1), `vbTileVertical` (2) or `vbArrangeIcons`
(3); the headless host records it as a trace line, the Win32 host sends the
`MDICLIENT` cascade, tile, or icon-arrange message. Unloading the `MDIForm`
first unloads its children (newest first) with `UnloadMode = vbFormMDIForm`
(4); a child that cancels keeps the `MDIForm` loaded. The Win32 host uses a
frame window with an `MDICLIENT` and `WS_EX_MDICHILD` children. Menu merging is
not implemented.

## Manual drag and drop

`Control.Drag vbBeginDrag` marks the control as dragged; `vbEndDrag` and
`vbCancel` clear it. The input script command `drag <source> <destination> [x y]`
raises `DragOver` (State `vbEnter`) then `DragDrop` on the destination control
or form with `Source` set to the dragged control. `Drag vbBeginDrag` also asks
the host for a real drag.

## Mouse input

`MouseDown`, `MouseUp` and `MouseMove` (`Button`, `Shift`, `X`, `Y`) are raised
for forms and controls; `X` and `Y` are in the scale units of the surface (the
control itself for a form or picture box, the form otherwise). The Win32 host
subclasses every hosted window to observe the mouse; the input script commands
`mousedown`, `mouseup` and `mousemove <target> [button x y]` (twips) do the same
headless. A control with `DragMode = Automatic` starts a drag on `MouseDown`;
releasing the button over a control or form raises `DragOver` and `DragDrop`
there (headless: `mouseup` on the destination). OLE drag and drop is not
implemented.

## Printer and Printers

`Printer` is a virtual printer (`DeviceName` "WFC Virtual Printer", Letter paper,
`Orientation`, `Copies`, `Page`, font properties, `TextWidth`/`TextHeight`
estimates). `Printer.Print` accepts the full Print statement syntax and spools
text; `NewPage` starts the next page; `EndDoc` writes the document to the
Immediate output as `[Printer]` lines (with `-- new page --` markers) and
`KillDoc` discards it. `Printers.Count` is 1 and `Printers(0)` is `Printer`.
Graphics methods on the printer and `PrintForm` are not implemented in the Win32 host.

## OLE drag and drop (headless model)

`DataObject` (`SetData`, `GetData`, `GetFormat`, `Clear`, `Files`) and the
`vbDropEffect*`, `vbOLEDrag*` and `vbOLEDrop*` constants exist. The input script
command `oledrag <source> <destination> [x y]` runs the sequence of an OLE drag:
`OLEStartDrag(Data, AllowedEffects)` on the source (copy and move allowed), then,
when the destination's `OLEDropMode` is manual, `OLEDragOver` (state `vbEnter`)
and `OLEDragDrop` there with the effect the handler chose, and finally
`OLECompleteDrag(Effect)` on the source (effect 0 when nothing accepts the
data). Dragging between processes with a real `IDropSource`/`IDropTarget` is not
implemented.

## Drive, directory and file list boxes

`DriveListBox` lists the existing drive letters (`Drive`, Change; an unknown
drive raises 68). `DirListBox` shows the ancestors of `Path` and its
subdirectories (`List` returns full paths, the host shows indented names);
`Path` raises Change, a missing directory raises 76, and double-clicking an
entry makes it the current directory. `FileListBox` lists the files of `Path`
that match `Pattern` (semicolon-separated wildcards, `*.*` matching every
name), sorted without regard to case; `Path` raises PathChange, `Pattern`
raises PatternChange, selecting an item sets `FileName` and raises Click. The
`Archive`/`Hidden`/`ReadOnly`/`System`/`Normal` attribute filters are not
applied.

## Data binding (REQ-0131)

`TextBox`, `Label`, `CheckBox` and `ComboBox` bind to a class-module object:
`Set ctl.DataSource = obj`, `DataField` names a property or field of the object
and `DataMember` (optional) names a property returning the object to read
instead. Assigning `DataSource`, `DataField` or `DataMember` reads the field
into the control (`Label` and `CheckBox` take `Caption` and `Value`; `Null`
reads as empty) and clears `DataChanged`. Editing the control, from code or the
user, sets `DataChanged`; the edit is written back when the control loses the
focus and when the form unloads. A missing member raises the ordinary 438.
`DataFormat`, record navigation and DAO/ADO recordsets are not implemented
(DAO/Jet is excluded from 1.0).

## Unsupported forms (REQ-0115)

A form is checked when the project is loaded. A control class the runtime does
not provide (including every ActiveX control until MP-0005/MP-0006) is rejected
with `WFC0291`, and a property that refers to a missing or unreadable `.frx`
record with `WFC0292`; nothing is skipped silently.

## DPI

The Win32 host declares system DPI awareness before creating windows and
converts twips to pixels at the system DPI, so forms keep their VB layout on
scaled displays. Per-monitor DPI changes while a program runs are not tracked.

## `.frx` lists, `Controls.Add` and `WithEvents` controls

A `ListBox` or `ComboBox` `List` property stored in the `.frx` file (a 16-bit
count, then each item as a 16-bit length and its text) fills the items when the
form loads. `Controls.Add(ProgID, Name[, Container])` creates an invisible
intrinsic control (`VB.` prefix optional; Form, Menu and non-intrinsic classes
raise 711, a duplicate name 727) and `Controls.Remove(Name)` removes a control
added that way (361 otherwise). A control assigned to a `WithEvents` variable
raises its events to `variable_Event` handlers, in addition to any
`ControlName_Event` handler.

## Graphical buttons

A `CommandButton`, `CheckBox` or `OptionButton` with `Style = 1` shows its
`Picture` instead of its caption, from the `.frm` or when the property is
assigned (`UiHost::set_button_image`; the headless host records a `button image`
trace line). `DownPicture` and `DisabledPicture` are stored but not yet shown
separately.

## Known limits (extended later in MP-0004)

OLE drag and drop, printer graphics, `PrintForm`, and
MDI menu merging are not yet implemented.

## Verification

`TC-MP0002-corpus-151-form-basic`, `-152-form-timer-arrays`, `-153-form-lists`, `-154-form-scrollbars`, `-155-form-menus-dialogs`, `-156-form-modal`, `-158-form-navigation`, `-159-form-controls-collection`, `-160-form-end`, `-161-form-fonts-misc`, `-164-form-sendkeys`, `-167-form-mdi`, `-168-form-drag`, `-170-form-filelists`, `-171-form-databinding`, `-172-form-frx-list`, `-173-form-controls-add`, `-183-licenses`, `-184-form-mouse`, `-185-form-oledrag`, `-187-form-printform`, `-174-form-button-picture`, `169-printer`, `TC-MP0004-gui-mdi`, `TC-MP0004-form-unsupported-control`, `TC-MP0004-form-missing-resource`, `TC-MP0004-win32-host` and `TC-MP0004-gui-form-timer` and the `wfc_ui_tests` units.

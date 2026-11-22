# ADR-0012: VB6 Forms in Text Mode on wfc-brun71

**Content type:** Architecture decision record

**Status:** Accepted by the owner as milestone M3 of `lib/wfc-brun71`
(2026-11-22); step 1 of the implementation order is done

**Date:** 2026-11-22

## Context

M2 (ADR-0011) runs PDS 7.1 programs on the `lib/wfc-brun71` text screen. The
owner wants more than compatibility with old PDS programs: to write programs
in the VB6 language with an event-driven text user interface, as Visual Basic
for MS-DOS did. Real threads, and several programs at once, are wanted later,
not now.

WFC already has everything above the window system for this. Forms and
controls are a model (`src/interpreter/forms.cpp`) that talks to a window
system only through the `UiHost` interface (ADR-0005): windows, their bounds,
text, items, values, focus, timers, dialogs, and an event queue. Two hosts
exist: Win32 and the scripted headless host. Native programs run their forms
through the same model (ADR-0009). The library has the screen, the keyboard,
code page 437 and the Windows console host (M1), and WFC shares one screen per
process between the interpreter and native code (ADR-0011).

## Decision Drivers

- The same VB6 language, `.frm` files, events and control model as a
  windowed program, not a second dialect.
- One implementation of control logic (ADR-0005): no text-only copy of the
  form model.
- Tests without a desktop: keys from a file, the final screen as text, as the
  PDS corpus programs do.
- A single-threaded, event-driven program, as in VB6; nothing that makes
  later workers harder.

## Considered Options

1. **The VB-DOS forms language** (`.FRM` files with character coordinates and
   VB-DOS's own control set). Compatible with old VB-DOS sources, but a
   second dialect and a second form model.
2. **A text-mode object model for programs to call** (windows, menus and
   list boxes as classes, like the PDS User Interface Toolbox). No designer
   files, but programs would be written against a WFC-only API.
3. **A third `UiHost`, the text host,** that draws VB6 forms and controls on
   the text screen and turns keys and the console mouse into the events the
   form model already handles.

## Decision

Take option 3.

1. **The text host.** `ui::TextHost` (`src/ui/text_host.cpp`) implements
   `UiHost` on the process's `brun71::TextScreen` and keyboard. The form
   model, the event path and the controls' properties are unchanged. The
   library gets what does not depend on WFC: drawing boxes, shadows and
   control faces on a `TextScreen` region, and the line editor of M2 for
   text boxes (`forms.hpp` in the library).
2. **How a project asks for it.** A `.vbp` line `WfcTextUI=1` (as
   `WfcUiScript=` is a WFC key) or `wfc --text-ui` for any project selects the
   text host (`ui_mode` `"text"`). `wfc build` of such a project writes a
   console program, not a `--gui` one. The screen is shown as in M2: live in
   the console, or the final screen printed as text when the output is
   redirected, with keys from the console or the standard input.
3. **Coordinates.** A character cell is 120 twips wide and 240 twips high,
   which is VB6's `ScaleMode = 4` (vbCharacters), so a form designed in
   characters keeps its layout. Twips round to the nearest cell; a visible
   control is at least one cell. The screen is 80 by 25 cells (`WIDTH` may
   change it); `Screen.Width` and `Screen.Height` report it in twips.
4. **The look** follows the text-mode windowing of DOS programs: a form is a
   box with its caption on the top border, a double border when active, and
   a shadow; `BackColor` and `ForeColor` map to the nearest of the 16 CGA
   colours. A `CommandButton` is `< Caption >`, a `CheckBox` `[X]`, an
   `OptionButton` `(*)`, a `TextBox` a field in its own colour, a `Frame` a
   single box, a `ListBox` and the list of a `ComboBox` a box with a scroll
   bar, scroll bars of block characters. The exact characters are decided in
   the library with their tests, not here.
5. **Input.** Tab and Shift+Tab move the focus in `TabIndex` order; Enter
   presses the `Default` button and Esc the `Cancel` one; Space presses a
   button or toggles a check box; arrow keys move among option buttons and in
   lists; Alt and the letter after `&` in a caption are access keys. Key
   presses go to the focused control as `KeyDown`, `KeyPress` and `KeyUp`
   first, as in VB6 (`KeyPreview` included). Console mouse input becomes
   `MouseDown`, `MouseUp`, `MouseMove` and `Click` at the cell's twips.
6. **Events, not threads.** `next_event(wait)` waits on the console's input
   and the host's timers; `Timer` controls and `DoEvents` work as in a
   windowed program. Program code runs on one thread (ADR-0004). Workers with
   their own interpreter that pass messages as events are a later decision.
7. **Menus and dialogs.** A form's `Menu` controls become a menu bar on the
   form's first row; Alt or F10 opens it, and shortcut keys work while the
   form is active. `MsgBox` and `InputBox` are text dialogs through the
   host's `message_box` and `input_box`. The common dialogs report that the
   host has none, as the headless host does.
8. **What a text screen cannot show.** Pictures, graphics methods (`Line`,
   `Circle`, `PSet`, `PaintPicture`) and ActiveX controls are ignored by the
   host, with one warning per kind in the Immediate output; `Print` on a form
   writes text at `CurrentX` and `CurrentY` in cells. The MSComctlLib
   controls and MDI forms are later work.
9. **PDS statements alongside.** The screen is one: PDS statements of
   ADR-0011 in standard modules write to it, and the text host repaints its
   forms over it when a form changes. A program normally uses one or the
   other.

## Consequences

- A VB6 form project runs as a text program by adding `WfcTextUI=1`, in the
  interpreter and as native code, with no change to its code.
- Corpus programs check text forms as the PDS ones do: a `.keys` file drives
  them and the expected output is the final screen.
- The `UiHost` interface may need small additions for things only a text
  screen needs (for example the access-key map); they follow ADR-0005's
  review rule.
- Layout that relies on proportional fonts or pixel sizes looks coarser in
  cells; forms meant for text should be designed in `ScaleMode = 4`.
- The PDS User Interface Toolbox (`MENU.BAS`, `WINDOW.BAS`) can later be
  built on the same drawing code.

## Implementation Order

1. The host skeleton: forms as boxes, `Label`, `CommandButton`, focus,
   keys from the standard input, the final screen as text.
2. `TextBox`, `CheckBox`, `OptionButton`, `Frame`.
3. `ListBox`, `ComboBox`, scroll bars, `Timer`, `DoEvents`.
4. Menus, `MsgBox`, `InputBox`.
5. The console mouse; `wfc build` as a console program; native-code corpus
   runs of every case.

## References

- [ADR-0005: UI platform abstraction](adr-0005-ui-platform.md)
- [ADR-0009: variants, objects and forms in native programs](adr-0009-variants-objects-and-forms-in-native-programs.md)
- [ADR-0011: PDS 7.1 text-mode programs](adr-0011-pds-text-programs.md)
- `lib/wfc-brun71/ROADMAP.md`, milestone M3

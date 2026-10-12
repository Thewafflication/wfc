# REQ-0294 — `UserControl` modules hosted by forms of the project

**Content type:** Project requirement

**Status:** Accepted (first increment; ActiveX exposure of a UserControl is a later MP-0005 increment)

## Statement

- **Project item.** `UserControl=File.ctl` loads a control module: `VERSION`,
  `Begin VB.UserControl Name` with constituent controls, `Attribute VB_Name`,
  and code. It has no default instance.
- **Hosting.** A form block `Begin Library.Name Control1` (the library prefix is
  not checked) hosts the UserControl `Name` in a child window. The form
  variable is a container extender (`ADR-0006`): the extender members of
  `REQ-0293` belong to the container and every other member goes to the
  control's own public properties, methods and fields by name; `.Object` is the
  control. `TypeName` is the control's class name.
- **Events.** A `Public Event` of the control is raised with `RaiseEvent` and
  reaches `Control1_Event` handlers of the form.
- **Lifecycle.** `UserControl_Initialize` (when the instance is created), then
  `ReadProperties(PropBag As PropertyBag)` with a bag holding the form-file
  properties the container does not own, then `Resize` and `Show`;
  `Resize` follows every `Width` or `Height` change of the extender;
  `Terminate` runs when the host form unloads. Inside the control
  `Extender` returns the host's extender, `Width`/`Height` are its size, and the
  constituent controls behave as on a form.
- **`Ambient` and `PropertyChanged`.** `Ambient` returns the container's ambient
  properties (system colors, `MS Sans Serif` 8.25, `UserMode` True, locale 1033,
  `ScaleUnits` "Twip"); `PropertyChanged` is accepted and does nothing.
- **Form and MDI form `Initialize`.** `Form_Initialize` and `MDIForm_Initialize`
  run when the form instance is created (before `Load`).

## Known limits

`InitProperties`, `WriteProperties`, `Paint`,
`AccessKeys`, windowless and `Public` constituent controls, design-time property
pages and exposure of a UserControl as an ActiveX control to other containers
are not implemented. Custom property values come from plain lines in the form
file, not from an `.frx` blob.

## Verification

`TC-MP0002-corpus-180-form-usercontrol` and `TC-MP0005-gui-usercontrol`.

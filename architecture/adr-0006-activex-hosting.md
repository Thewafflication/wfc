# ADR-0006: ActiveX Control Hosting and the `MSComctlLib` Strategy

**Content type:** Architecture decision record

**Status:** Proposed (hosting accepted by implementation in MP-0005; the
`MSComctlLib` strategy awaits owner approval before MP-0006)

**Date:** 2026-10-11

## Context

Forms name ActiveX controls (`Begin MSComctlLib.TabStrip TabStrip1`,
`Begin SHDocVw.WebBrowser Web1`) after an `Object = "{GUID}#ver#flags"; "file"`
line. MP-0004 rejected them with `WFC0291`. MP-0005 must host them; MP-0006
must deliver the nine `MSComctlLib` controls. The real `MSCOMCTL.OCX` is a
32-bit component with no 64-bit build, so an x64 or ARM64 WFC process cannot
load it.

## Decision

1. **Container.** The Win32 host creates each ActiveX control in a child window
   of class `AtlAxWin` from the system `atl.dll`, which is a complete control
   container (creation from the CLSID, in-place activation, sizing, ambient
   properties, keyboard routing). The headless host creates the control with
   `CoCreateInstance` and no site, so conformance tests need no window system.
   The `UiHost` interface gains one method, `activex_object(window)`.
2. **Extender.** The form variable for an ActiveX control is a `ControlObject`
   of the internal class `ActiveXControl` that owns the container-side members
   (`Left`, `Top`, `Width`, `Height`, `Visible`, `TabIndex`, `TabStop`, `Tag`,
   `ToolTipText`, `Name`, `Parent`, `Container`, `Move`, `SetFocus`, `ZOrder`,
   `Drag`, and the focus and drag events) and forwards every other member to
   the control through `IDispatch`. `.Object` returns the control itself.
3. **Events.** The control's outgoing interface is connected with the MP-0003
   event-sink machinery; events call the form's `ControlName_Event` handlers or
   a `WithEvents` variable.
4. **Class resolution.** `Library.Class` in a form resolves through the type
   libraries named by the project's and the forms' `Object` lines
   (`Reference=` lines already work), using the coclass CLSID.
5. **`MSComctlLib` (proposal for MP-0006).** Implement the nine controls
   natively in WFC behind the same extender and register them under the
   `MSComctlLib` class names, so `Begin MSComctlLib.TabStrip` works on every
   architecture without the OCX. On x86, when the OCX is registered, a
   conformance mode can host the stock control instead and compare behavior;
   it is a test aid, not a runtime requirement. Native implementation also
   keeps the headless host able to test them.

## Consequences

- Any registered in-process control with an `IDispatch` interface can be hosted
  today; controls that need unsupported container interfaces fail with 429.
- Design-time property blobs in `.frx` files (`OleObjectBlob`) are not loaded
  yet; text properties in the `.frm` are applied to the control.
- Licensed controls need `Licenses` support (MP-0005 later increment).
- The native `MSComctlLib` route costs more implementation work than hosting
  the OCX but is the only one that satisfies the x64 and ARM64 targets.

## References

- [MP-0005 plan](../planning/MP-0005-component-authoring.md)
- [ADR-0004](adr-0004-automation-server.md), [ADR-0005](adr-0005-ui-platform.md)
- [REQ-0293](../requirements/language/req-0293-activex-control-hosting.md)

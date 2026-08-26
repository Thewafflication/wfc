# Visual Basic 6.0 Form and Intrinsic-Control Behavioral Requirements

**Content type:** Project requirement index

**Status:** Proposed

These requirements turn the installed VB6 form/control object model into
observable obligations. The core `VB98.CHM` help is not installed; the source
basis and substitution policy are recorded in the retained documentation
baseline. Each requirement remains Proposed until its named reference probe
confirms defaults, ordering, errors, rendering, and platform-sensitive behavior.

## Forms

- [REQ-0106 — Form instance and load lifecycle](req-0106-form-instance-lifecycle.md)
- [REQ-0107 — Form visibility, ownership, and modality](req-0107-form-visibility-modality.md)
- [REQ-0108 — Form query, unload, and termination](req-0108-form-unload-termination.md)
- [REQ-0109 — Activation, focus, validation, and key preview](req-0109-activation-focus-validation.md)
- [REQ-0110 — Form window geometry, state, and MDI behavior](req-0110-form-window-mdi.md)
- [REQ-0111 — Control containment, lookup, and control arrays](req-0111-containment-control-arrays.md)
- [REQ-0113 — Form and PictureBox coordinate and drawing behavior](req-0113-coordinates-drawing.md)
- [REQ-0115 — Form and intrinsic-control persistence](req-0115-form-control-persistence.md)

## Common control behavior

- [REQ-0112 — Common intrinsic-control state and interaction](req-0112-common-control-state.md)
- [REQ-0114 — Manual and OLE drag-and-drop behavior](req-0114-drag-drop.md)
- [REQ-0130 — VB6 DDE link behavior](req-0130-dde-links.md)
- [REQ-0131 — Common intrinsic-control data binding](req-0131-data-binding.md)

## Intrinsic controls

- [REQ-0116 — PictureBox container and presentation behavior](req-0116-picture-box.md)
- [REQ-0117 — Label caption and mnemonic behavior](req-0117-label.md)
- [REQ-0118 — TextBox editing and selection behavior](req-0118-text-box.md)
- [REQ-0119 — Frame containment and option grouping](req-0119-frame.md)
- [REQ-0120 — CommandButton activation behavior](req-0120-command-button.md)
- [REQ-0121 — CheckBox and OptionButton selection behavior](req-0121-selection-buttons.md)
- [REQ-0122 — ListBox and ComboBox item and selection behavior](req-0122-list-combo.md)
- [REQ-0123 — Horizontal and vertical ScrollBar behavior](req-0123-scroll-bars.md)
- [REQ-0124 — Timer scheduling behavior](req-0124-timer.md)
- [REQ-0125 — Image, Shape, and Line presentation behavior](req-0125-lightweight-graphics.md)
- [REQ-0126 — Drive, directory, and file list behavior](req-0126-file-system-lists.md)
- [REQ-0127 — Menu hierarchy and command behavior](req-0127-menus.md)
- [REQ-0128 — Intrinsic Data control behavior](req-0128-data-control.md)
- [REQ-0129 — Intrinsic OLE container behavior](req-0129-ole-control.md)

## Evidence and Acceptance

The type library establishes the callable surface but does not encode event
order, defaults, pixels, timing, errors, or persistence rules. A requirement
becomes eligible for acceptance only after its planned `TC-NNNN` probe records
the installed VB6 behavior and verifies WFC against the resulting expectation.

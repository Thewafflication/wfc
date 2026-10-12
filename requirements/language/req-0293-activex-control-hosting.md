# REQ-0293 — ActiveX control hosting on forms

**Content type:** Project requirement

**Status:** Accepted (first increment of MP-0005 control hosting)

## Statement

- **Declaration.** A form block `Begin Library.Class Name` whose library is
  named by an `Object = "{GUID}#major.minor#flags"; "file"` line (in the form or
  the project) hosts the coclass `Class` of that type library. A class the
  program cannot resolve is rejected at load with `WFC0291`.
- **Creation.** When the form loads the control is created and sited by the
  host (`ADR-0006`); failure raises 429. Design-time properties of the `.frm`
  that the container does not own are assigned to the control; unsupported ones
  are skipped.
- **Extender.** `Left`, `Top`, `Width`, `Height`, `Visible`, `TabIndex`,
  `TabStop`, `Tag`, `ToolTipText`, `HelpContextID`, `CausesValidation`,
  `DragMode`, `Name`, `Parent`, `Container`, `Move`, `SetFocus`, `ZOrder` and
  `Drag` belong to the container (twips, as for intrinsic controls) and
  `Object` returns the control's own object. Any other member is forwarded to
  the control (properties, methods, collections, default member).
- **Events.** The control's events call `Name_Event` handlers of the form.
- **Control arrays and dynamic controls.** ActiveX controls may be control
  array elements (`Index`, `Load`, `Unload`). `Controls.Add(ProgID, Name[,
  Container])` accepts the ProgID or CLSID of a registered control (the control
  is invisible until `Visible = True`); an unknown ProgID raises 711 and an
  uncreatable control 429. `As VBControlExtender` variables accept any hosted
  control (`ObjectEvent` and licensing are not implemented).

## OLE container control

`OLE.CreateEmbed(SourceDoc, Class)` creates an embedded object from a ProgID or
CLSID (the `Class` property when the argument is omitted) the way an ActiveX
control is created; `Object` returns it, `OLEType` is 1 (embedded) or 3 (none),
`AppIsRunning` tells whether an object is held and `Close`/`Delete` release it.
`DoVerb`, `Update`, `Copy`, `Paste` and `FetchVerbs` are accepted and do
nothing. An unknown class raises 31018. Linked objects, `InsertObjDlg`,
`PasteSpecialDlg`, `SaveToFile`/`ReadFromFile`, in-place activation of the
embedded object's UI and the `Updated` event are not implemented.

## Known limits

`OleObjectBlob` persistence data in `.frx` files, ambient property overrides,
licensing, keyboard accelerators and tab navigation into hosted controls, and
screenshot capture of controls with their own composition surface (the hosted
WebBrowser) are not implemented.

## Verification

`TC-MP0002-corpus-177-form-activex`, `-178-form-controls-add-activex`, `-179-form-activex-array`, `-186-form-ole` (headless, `MSXML2.DOMDocument60` as the
hosted object with its events) and `TC-MP0005-gui-activex` (a hosted
`SHDocVw.WebBrowser` raising `DocumentComplete` in a real window).

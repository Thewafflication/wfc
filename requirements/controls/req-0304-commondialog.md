# REQ-0304 — Common Dialog control (`MSComDlg.CommonDialog`)

**Content type:** Project requirement

**Status:** Proposed (no VB6 reference probe; behavior follows the documented
control)

## Statement

- **Class.** `Begin MSComDlg.CommonDialog` forms (and `Dim d As
  MSComDlg.CommonDialog`) resolve to a native, non-visual control; no OCX is
  needed. The `Object=` lines of `comdlg32.ocx` are accepted.
- **Properties.** `Action`, `CancelError`, `Color`, `Copies`, `DefaultExt`,
  `DialogTitle`, `FileName`, `FileTitle`, `Filter`, `FilterIndex`, `Flags`,
  `FontBold`, `FontItalic`, `FontName`, `FontSize`, `FontStrikethru`,
  `FontUnderline`, `FromPage`, `HelpCommand`, `HelpContext`, `HelpFile`,
  `HelpKey`, `InitDir`, `Max`, `MaxFileSize`, `Min`, `Orientation`,
  `PrinterDefault`, `ToPage`, `hDC` hold their values with VB defaults.
- **Methods.** `ShowOpen`, `ShowSave`, `ShowColor`, `ShowFont`, `ShowPrinter`,
  `ShowHelp`; assigning 1 to 6 to `Action` runs the same dialog.
- **Cancel.** When the user cancels and `CancelError` is true, error 32755
  ("Cancel was selected") is raised; otherwise the properties are unchanged.
  `ShowFont` without `cdlCFScreenFonts` or `cdlCFPrinterFonts` in `Flags` raises
  24574 ("No fonts exist"). `ShowPrinter` has no dialog yet and is cancelled.
- **Win32 host.** The system dialogs (`GetOpenFileName`, `GetSaveFileName`,
  `ChooseColor`, `ChooseFont`) run; a chosen file sets `FileName`, `FileTitle`
  and `FilterIndex`, a colour sets `Color`, a font sets the `Font*` properties.
- **Headless host and scripts.** A scripted `answer open|save <path>`,
  `answer color <number>` or `answer font Name,Size,Bold,Italic` stands for the
  user's choice; `answer <kind> cancel`, or no answer, cancels.
- **Constants.** `cdlCancel`, `cdlOFN*`, `cdlCF*`, `cdlCC*`.

## Known limits

No printer dialog, no help launch, no hook or template flags, no multi-select
file lists, `hDC` is zero. The Win32 dialogs are not covered by automated tests
(they are modal); the scripted path is (`196-ocx-winsock-dialog`).

## Verification

Corpus `196-ocx-winsock-dialog`.

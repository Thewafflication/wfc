# REQ-0291 — Picture objects

**Content type:** Project requirement

**Status:** Accepted (first increment; the full `stdole` contract is MP-0005)

## Statement

- **`StdPicture`.** `LoadPicture(path)` returns a picture object for BMP,
  ICO/CUR, GIF, JPEG, PNG, WMF (placeable) and EMF files; a missing file raises
  53 and unrecognized data 481. `LoadPicture("")` is an empty picture. A picture
  has `Handle` (nonzero, stable; 0 when empty), `Type` (`vbPicTypeBitmap` 1,
  metafile 2, icon 3, enhanced metafile 4), `Width` and `Height` in HIMETRIC
  (0.01 mm; the headless host assumes 96 dpi) and is read-only. `Handle` is the
  default member. `SavePicture picture, path` writes the original image bytes.
- **Picture properties.** `Picture`, `Icon`, `MouseIcon`, `DragIcon`,
  `DownPicture` and `DisabledPicture` hold pictures; reading one that was never
  set gives an empty picture; assigning `Nothing` clears it; assigning anything
  else raises 13. Pictures in a `.frm` come from the `.frx` file.
- **Display.** A form or picture box draws its `Picture` at the origin at
  natural size when shown or assigned, and after `Cls`. A picture box with
  `AutoSize` resizes to its picture (plus a border when it has one).
- **`App.Path`** is the project's folder when a `.vbp` is run.
- `As StdPicture`, `As Picture` and `As IPictureDisp` are accepted types.

- **Lightweight controls.** `Image` (natural size, or stretched to its box with
  `Stretch`), `Shape` (types 0-5, `BorderColor`, `FillColor`, `FillStyle`,
  `BorderWidth`, `BorderStyle`) and `Line` (`X1`..`Y2`, `BorderColor`) have no
  window; the form or picture box that contains them draws them. Loading the form
  or changing a lightweight control repaints the surface (clear, picture, then the
  controls in creation order), so drawings the program made on it are lost, as when
  a form without `AutoRedraw` repaints. `Cls` keeps the controls.

## Known limits

`PaintPicture`, `Point` and picture properties of command buttons and option
buttons do not display pictures yet.

## Verification

`TC-MP0002-corpus-162-form-pictures`, `-163-form-lightweight` and `TC-MP0004-win32-host`.

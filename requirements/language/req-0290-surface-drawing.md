# REQ-0290 — Drawing surfaces: coordinates, graphics statements, `Print`

**Content type:** Project requirement

**Status:** Accepted (first increment; pictures and AutoRedraw semantics follow)

## Statement

A form and a `PictureBox` are drawing surfaces.

- **Coordinates.** `ScaleMode` (`vbUser` 0, `vbTwips` 1 (default), `vbPoints`,
  `vbPixels`, `vbCharacters`, `vbInches`, `vbMillimeters`, `vbCentimeters`)
  fixes the unit; `ScaleLeft`, `ScaleTop`, `ScaleWidth`, `ScaleHeight` read the
  current scale and, when assigned (or set by the `Scale (x1,y1)-(x2,y2)`
  statement), switch the mode to user. `Scale` alone restores twips. A design-time
  scale that differs from the client area is a user scale. `ScaleX` and
  `ScaleY` convert between modes. `CurrentX` and `CurrentY` hold the position in
  scale units.
- **Statements and methods.** `PSet`, `Line` (with `Step`, `B`, `BF`, a
  start omitted to continue from the current position), `Circle` (radius,
  color, arc start and end, aspect), `Cls`, `Scale` and `Print`, as statements
  inside the form's own code and as `Surface.Method` calls. Colors default to
  `ForeColor`; system color values map to stock colors. Operations are sent to
  the host in twips.
- **`Print` and text.** `Print` on a form or picture box (including a bare
  `Print` in form code) draws at `CurrentX/CurrentY` with the surface font, then
  moves to the next line (or along the line after a trailing `;`). The
  headless host also writes the printed text to the program output; the Win32
  host only draws. `TextWidth` and `TextHeight` measure in scale units; the
  headless host uses a deterministic proportional model.
- **Headless trace.** The headless host records each operation as a line
  (`pset #id x y color`, `line #id x1 y1 x2 y2 color [B|BF]`,
  `circle #id x y r color [start end] [aspect a]`, `text #id x y color "text"`,
  `cls #id`); the input script's `trace` command prints and clears them.

- **`Point` and `PaintPicture`.** `Point(x, y)` returns the color of the surface
  pixel (-1 outside the surface); the headless host keeps a software copy of the
  surface (one pixel per 15 twips) updated by `Cls`, `PSet`, `Line`, `Circle` and
  shapes (text and pictures leave no pixels). `PaintPicture picture, x1, y1
  [, width1, height1 [, x2, y2, width2, height2]]` draws a picture (or part of it)
  into a rectangle in scale units.

## Known limits

`DrawWidth`, `DrawStyle`, `FillColor`, `FillStyle`, `DrawMode`, `AutoRedraw` and the `Paint`
event's repaint behavior are not implemented yet; the Win32 host draws
persistently on a backing surface (as `AutoRedraw = True`).

## Verification

`TC-MP0002-corpus-157-form-graphics` and `-165-form-point-paintpicture`.

# Control render: how the MSComctlLib controls look

One form (`frmRender.frm`, project `ControlRender.vbp`) with a Toolbar, a
TabStrip, a ProgressBar, a Slider, a TreeView, a ListView and a StatusBar,
filled in `Form_Load` with the same code under every runtime. The caption
reports the tab and panel counts the controls start with.

| image | what ran it |
|-------|-------------|
| `vb6.png` | VB6 (6.00.8176) compiled exe, run on Windows 11 |
| `wfc-windows-x64.png` | `wfc.exe --interpret --gui`, x64 release, Windows 11 |
| `wfc-wine.png` | the same `wfc.exe` under Wine 8.0 in WSL (WSLg display) |
| `compare.png` | the three side by side |

Windows shots are the window copied from the screen. The Wine shot is made by
`grab.c` (built with `wineg++`), which copies the window DC; the black bar at
its top is the caption that the window manager draws, not the form.

## Differences seen

1. **Start state (fixed).** VB6 starts the TabStrip with 1 tab and the
   StatusBar with 1 panel (`initial tabs=1 panels=1`). WFC started both empty,
   so `Tabs(1)` and `Panels(1)` failed with 35600. A control loaded from a
   form file with no persisted tabs or panels now starts with one, and
   `wfc-windows-x64.png` shows `tabs=1 panels=1`.
2. **Toolbar.** VB6 draws raised 3-D buttons; WFC draws flat text buttons.
3. **TreeView.** A TreeView loaded from a form without a Style is text only
   in VB6 (Style 0); WFC draws lines and plus/minus boxes (Style 7). This is
   one of the known differences in `../control-probe`.
4. **Look.** A VB6 exe has no manifest, so its controls are the classic
   (comctl32 v5) ones; `wfc.exe` is manifested for themed (v6) controls. The
   ProgressBar, Slider and ListView header are close; tabs and the status
   bar differ in the 3-D edges.
5. **Wine.** Wine draws its own controls: a solid blue ProgressBar (VB6 and
   Windows WFC draw segments), flat tabs and sunken frames with a thin line.
   The layout, texts, tree, list and status panels are the same as Windows.
6. **`--screenshots` under Wine (fixed).** The PNG that `wfc.exe` wrote itself
   came out blank grey under Wine, because `PrintWindow` does not render the
   window there. Under Wine the host now copies the window DC, as `grab.c`
   does.

## Winelib

`wfc-winelib.png` is made by a native Linux build of wfc (an ELF shared object
that Wine loads), compiled and linked with `wineg++` (g++ 12) against Wine's
Windows libraries. It is pixel-identical to `wfc.exe` under Wine, and its 741
lines of the control probe are identical to the Windows x64 build's.

Build (WSL, Wine 8.0 development packages, CMake 3.28 or newer; no sudo):

```
cmake -S . -B out/winelib -DCMAKE_TOOLCHAIN_FILE=cmake/winelib.cmake \
      -DWFC_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build out/winelib --target wfc
DISPLAY=:0 wine64 out/winelib/wfc.exe.so --interpret <project.vbp> --gui
```

What the port needed (`src/winelib_compat.hpp`, `cmake/winelib.cmake`):

- the secure CRT functions glibc lacks (`localtime_s`, `_dupenv_s`,
  `fopen_s`, `wcsncpy_s`, `IID_PPV_ARGS`);
- winegcc's 16-bit `wchar_t` against glibc's 32-bit `wcslen` and `wmem*`
  (libstdc++ calls them from `std::wstring`) and `_GLIBCXX_EXTERN_TEMPLATE=0`;
- `_WIN32` hidden while `<filesystem>` is included, or its paths are wide;
- `-fPIC`, no PIE hardening (winegcc links a shared object), the `version`,
  `uuid` and `dbghelp` libraries, `CreateThread` for the large-stack thread,
  the stack limits from the TIB, and `dladdr` for the program's own path.

Not done: paths. The program is a Linux program, so a probe that builds a path
with `App.Path & "\file"` writes a file named `dir\file`; WFC treats `\` as a
separator only on Windows. `--version`, interpreting a project, forms and
screenshots work.

## Geometry (frmGeo.frm, GeometryProbe.vbp)

`geometry-vb6.txt` is what VB6 reports for the same controls (twips: the
pixels in whole himetric, so 36 pixels are 540.2835). WFC's output is now
identical to it. A Toolbar is raised buttons with the text below, 38 pixels
high and at least 36 wide, 2 pixels inside, fitted to its rows, and reports
those sizes as ButtonHeight and ButtonWidth; a Tab is 18 pixels high, its
text plus 13 wide, in container coordinates, and the client area is 4 pixels
in below the rows of tabs; a TreeView read from a form without `Style` is
`tvwTextOnly` with an indentation of 38 pixels.

## Winelib comparison (Wine 8.0, `wfc.exe.so` built with wineg++)

`geometry-winelib.txt` is the geometry probe run by the Winelib build: it is
identical to `geometry-vb6.txt` (the sizes come from WFC's own layout, not from
the Wine controls). `wfc-winelib.png` against `wfc-windows-x64.png` and
`vb6.png`: the controls sit at the same places with the same text, and the
toolbar buttons have the text below the picture room VB6 reserves. What differs
is Wine's own drawing: its Toolbar draws no raised button edges, its
ProgressBar is a smooth blue bar instead of blocks, and the tab, tree and list
borders are Wine's. These are Wine's common controls, not WFC.

The probe writes `App.Path & "\geometry.txt"`; under Winelib that is a file
named `<dir>\geometry.txt` in the current folder (Unix paths).

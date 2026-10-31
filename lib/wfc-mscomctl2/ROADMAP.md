# wfc-mscomctl2 roadmap

Library: **Windows Common Controls-2** (`wfcmscomctl2.dll`), the WFC
replacement for `mscomct2.ocx`. Current version **0.6.0**, targeting
**MSCOMCT2.OCX 6.00.8177 (Microsoft Common Controls-2, Visual Basic 6.0)**.

The version stays `0.<target major>.<target minor>` until the library matches
the Microsoft one functionally. A checked item is present, tested and passing;
an unchecked one is planned.

## 0.6.x: where it is

- [x] The member tables of the five controls, generated from the installed
      component's type library.
- [x] Version and target stamped into the DLL; `DllRegisterServer` and
      `DllUnregisterServer` (WFC ProgIDs and class ids; the Microsoft ones on
      request).
- [x] A C program tests the DLL; a table test checks the classes.
- [x] Standalone build; CI workflow for Windows, Winelib and WebAssembly.
- [ ] The library moves to a repository of its own (`wfc-mscomctl2`) and
      becomes a submodule of WFC.

## 0.7: the controls in the evaluator

One control at a time, each with a corpus program, a GUI example and a
requirement:

- [ ] UpDown (Value, Min, Max, Increment, Wrap, Orientation, the buddy control,
      `Change`, `UpClick`, `DownClick`).
- [ ] FlatScrollBar (Min, Max, Value, LargeChange, SmallChange, Orientation,
      `Change`, `Scroll`).
- [ ] MonthView (Value, the date range, selections, `DateClick`, `DateDblClick`,
      `GetDayBold`, `SelChange`).
- [ ] DTPicker (Value, Format, CustomFormat, CheckBox, UpDown, `Change`,
      `CloseUp`, `DropDown`).
- [ ] Animation (Open, Play, Stop, Close, AutoPlay, Center; AVI files).
- [ ] `DllGetClassObject` creates the controls without the WFC evaluator.

## 0.8: Microsoft ids verified

- [x] The class ids and ProgIDs come from the installed component's type
      library.
- [ ] Registration over the Microsoft ids tested against the real component.
- [ ] A program built against the Microsoft type library loads this library.

## 0.9: behaviour parity, 1.0: Microsoft parity

- [ ] Every member of [COMPATIBILITY.md](COMPATIBILITY.md) done or not
      applicable, each with a test; differences documented.
- [ ] Version takes the Microsoft version number; signed release and package.

## Platform track

- [ ] Windows x86, x64, ARM64 (MSVC) passing in CI.
- [ ] Linux x86, x64 and ARM through Winelib (`cmake/winelib.cmake`).
- [ ] WebAssembly with Emscripten: the member table and its page
      (`web/index.html`).

## Toolchain track

- [ ] Built with WFC's own C++ compiler and C++ library (neither exists yet).

## WSP track

- [x] WSP baseline (see [docs/WSP-ADOPTION.md](docs/WSP-ADOPTION.md)).
- [ ] The `wsp` submodule when the library has a repository of its own.
- [ ] 80-column source style for the library sources (the generated tables are
      exempt).
- [ ] Requirements and traceability (`requirements/`).

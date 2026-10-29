# wfc-comdlg32

Common Dialog for WFC: the native replacement for `comdlg32.ocx` (library MSComDlg).

- `src/comdlg32_tables.cpp` the member table of the control.
- `include/wfc/comdlg32/comdlg32.hpp` the accessor `wfc::ui::comdlg32_classes()`.
- `include/wfc/comdlg32/comdlg32.inc` the behaviour of the control: the
  properties, methods and events, compiled into the WFC evaluator, which
  includes it (`src/interpreter/forms.cpp`). It uses the evaluator's internal
  types, so it builds only inside WFC.
- `include/wfc/ocx/control_class.hpp` the table types, identical in every WFC
  control library (a WFC test checks the copies).
- `VERSION` the library version, stamped into `wfccomdlg32.dll`.

Target: `wfc_comdlg32` (static library, or `wfccomdlg32.dll` with `WFC_COMDLG32_SHARED=ON`;
the parent default is `WFC_OCX_SHARED`).

Part of [WFC](https://github.com/Thewafflication/wfc), where it is mounted at
`lib/wfc-comdlg32`. Licence: GPL-3.0-or-later (see the WFC `LICENSE`).

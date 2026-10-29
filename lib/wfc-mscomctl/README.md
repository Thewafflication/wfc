# wfc-mscomctl

Windows Common Controls for WFC: the native replacement for `mscomctl.ocx` (library MSComctlLib).

- `src/mscomctl_tables.cpp` the member table of the control, generated from the type-library evidence by `scripts/New-UiControlTables.ps1` in WFC.
- `include/wfc/mscomctl/mscomctl.hpp` the accessor `wfc::ui::mscomctl_classes()`.
- `include/wfc/mscomctl/mscomctl.inc` the behaviour of the control: the
  properties, methods and events, compiled into the WFC evaluator, which
  includes it (`src/interpreter/forms.cpp`). It uses the evaluator's internal
  types, so it builds only inside WFC.
- `include/wfc/ocx/control_class.hpp` the table types, identical in every WFC
  control library (a WFC test checks the copies).
- `VERSION` the library version, stamped into `wfcmscomctl.dll`.

Target: `wfc_mscomctl` (static library, or `wfcmscomctl.dll` with `WFC_MSCOMCTL_SHARED=ON`;
the parent default is `WFC_OCX_SHARED`).

Part of [WFC](https://github.com/Thewafflication/wfc), where it is mounted at
`lib/wfc-mscomctl`. Licence: GPL-3.0-or-later (see the WFC `LICENSE`).

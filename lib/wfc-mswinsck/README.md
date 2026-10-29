# wfc-mswinsck

Winsock for WFC: the native replacement for `mswinsck.ocx` (library MSWinsockLib).

- `src/mswinsck_tables.cpp` the member table of the control.
- `src/winsock_io.cpp` the non-blocking socket layer; it stays in the program
  (target `wfc_mswinsck_io`), not in the DLL.
- `include/wfc/mswinsck/mswinsck.hpp` the accessor `wfc::ui::mswinsck_classes()`.
- `include/wfc/mswinsck/mswinsck.inc` the behaviour of the control: the
  properties, methods and events, compiled into the WFC evaluator, which
  includes it (`src/interpreter/forms.cpp`). It uses the evaluator's internal
  types, so it builds only inside WFC.
- `include/wfc/ocx/control_class.hpp` the table types, identical in every WFC
  control library (a WFC test checks the copies).
- `VERSION` the library version, stamped into `wfcmswinsck.dll`.

Target: `wfc_mswinsck` (static library, or `wfcmswinsck.dll` with `WFC_MSWINSCK_SHARED=ON`;
the parent default is `WFC_OCX_SHARED`).

Part of [WFC](https://github.com/Thewafflication/wfc), where it is mounted at
`lib/wfc-mswinsck`. Licence: GPL-3.0-or-later (see the WFC `LICENSE`).

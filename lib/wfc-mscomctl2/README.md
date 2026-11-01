# wfc-mscomctl2

Windows Common Controls-2 for WFC: the native replacement for `mscomct2.ocx`
(library MSComCtl2): **Animation**, **UpDown**, **MonthView**, **DTPicker** and
**FlatScrollBar**.

Part of [WFC](https://github.com/Thewafflication/wfc), where it is mounted at
`lib/wfc-mscomctl2` (a submodule, like the other libraries). Licence:
GPL-3.0-or-later (see the WFC `LICENSE`).

What is here:

- `src/mscomctl2_tables.cpp` the member table of the five controls
  (214 members and 63 events), generated from the type library of
  the installed `MSCOMCT2.OCX` (`evidence/reference/mscomct2-2.0-type-library.json`
  in WFC) by `scripts/New-UiControlTables.ps1`.
- `include/wfc/mscomctl2/mscomctl2.hpp` the accessor
  `wfc::ui::mscomctl2_classes()`.
- `include/wfc/mscomctl2/mscomctl2.inc` the behaviour of the controls for the
  WFC evaluator: **not written yet** (the evaluator does not create these
  controls; see [COMPATIBILITY.md](COMPATIBILITY.md)).
- `include/wfc/ocx/control_class.hpp` and `registration.hpp`, identical in every
  WFC control library (a WFC test checks the copies).
- `VERSION` and `TARGET`: the version, stamped into `wfcmscomctl2.dll`, and the
  Microsoft component it stands in for.

Target: `wfc_mscomctl2` (static library, or `wfcmscomctl2.dll` with
`WFC_MSCOMCTL2_SHARED=ON`; the parent default is `WFC_OCX_SHARED`).

## Roadmap, compatibility and platforms

- [ROADMAP.md](ROADMAP.md): the controls to implement, in order, and the
  platform and toolchain tracks.
- [COMPATIBILITY.md](COMPATIBILITY.md): the surface of the type library and how
  far the library goes.
- [docs/WSP-ADOPTION.md](docs/WSP-ADOPTION.md): the WSP baseline.

## Version and target

The version is in `VERSION` and the Microsoft component it stands in for in
`TARGET`: **MSCOMCT2.OCX 6.00.8177 (Microsoft Common Controls-2, Visual Basic 6.0)**. The version stays `0.<target major>.<target minor>` until
the library matches the Microsoft one functionally. Both are stamped into the
DLL's version resource (`FileVersion`, `Comments`).

## Registration with Windows

`regsvr32 wfcmscomctl2.dll` (`/u` to remove) writes
`Software\WFC\Libraries\mscomctl2` (version, target, path) and, for every
control, a class under WFC's own ProgID and class id
(`WFC.MSComCtl2.<Class>`, the class id is a UUID derived from the ProgID) with
the DLL as in-process server. `WFC_REGISTER_SCOPE=user|machine` picks the
registry. With `WFC_REGISTER_MICROSOFT_IDS=1` the controls are also served under
the Microsoft ProgIDs and class ids (`MSComCtl2.DTPicker.2` and so on; they
come from the type library of the installed component, so they are checked);
what was registered there before is saved and restored by `regsvr32 /u`. The
library does not create the controls itself yet: `DllGetClassObject` reports
that the class is not available.

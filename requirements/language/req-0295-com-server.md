# REQ-0295 — WFC classes served as COM objects

**Content type:** Project requirement

**Status:** Accepted (late-bound server; implements `ADR-0004` option 2)

## Statement

- **Exposed classes.** Classes whose header has `Attribute VB_Exposed = True`
  and `Attribute VB_Creatable = True` are served. The ProgID is
  `ProjectName.ClassName` (`Name=` of the project, `Project1` by default) and
  the class id is derived from the ProgID, so it is the same on every run.
- **Modes.** `wfc project.vbp --serve` registers the classes as in-process class
  objects for the run, so the program's own `CreateObject("Project.Class")`
  creates them. `wfc project.vbp --server` also registers them for other
  processes and serves calls until the last object is released and no lock is
  held, or `--server-timeout <ms>` (default ten minutes) passes; COM's
  `-Embedding` argument selects the same mode. A project made only of class
  modules needs no standard module.
- **Registration.** `--register` and `--unregister` write and remove the ProgID
  and `CLSID` (with `LocalServer32` = `wfc <project> --server`) under
  `HKCU\Software\Classes`, and print each ProgID and class id. Nothing else
  touches the registry.
- **Type information and events.** The server builds a type library for the
  exposed classes (a dispatch interface `_Class`, an event interface
  `__Class` with the class's `Public Event` declarations, and the coclass with
  both as default interfaces; every parameter and result is a Variant, names
  are lower case, and a default member gets DISPID 0). It is written to
  `%TEMP%\wfc_<project>.tlb` when the server starts, or to the path given by
  `--write-typelib <path>`, which writes it, prints `LIBID {guid}` and exits.
  A client project can name it with `Reference=*\G{LIBID}#1.0#0#path#Name` for
  early binding and `WithEvents`. Objects implement `IConnectionPointContainer`
  (one connection point for the event interface), `IProvideClassInfo2` and
  `GetTypeInfo`; `RaiseEvent` calls every advised client sink.
- **Dispatch.** Each object implements `IDispatch`: public
  methods, `Property Get/Let/Set`, public fields and the default member are
  reachable by name; arguments and results use the Automation Variant
  conversions of `REQ-0286`; a VB object returned by a member becomes a new
  dispatch object. A run-time error in a member returns `DISP_E_EXCEPTION`
  with the VB number as `0x800A0000 | number` and the description.
- **Threading.** Calls run on the interpreter's apartment thread, between
  statements of the serving loop.

## Known limits

Only Variant types in the type library (no typed parameters, `Enum`s or
`Type`s), no `IDispatchEx`, no `Instancing` variants other than creatable
multi-use, no automatic reference-based termination of `Class_Terminate`, and
no in-process DLL servers (MP-0007). Per-user `LocalServer32` auto-start is
not exercised by the tests: on the development machine COM refused to start
per-user servers, so the process test starts the server itself.

## Verification

`TC-MP0005-com-server-inproc`, `TC-MP0005-com-server-process` (a server
process and a client process) and `TC-MP0005-com-server-early` (an early-bound
client with `WithEvents` against the exported type library).

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
- **Dispatch.** Each object implements `IDispatch` (no type information): public
  methods, `Property Get/Let/Set`, public fields and the default member are
  reachable by name; arguments and results use the Automation Variant
  conversions of `REQ-0286`; a VB object returned by a member becomes a new
  dispatch object. A run-time error in a member returns `DISP_E_EXCEPTION`
  with the VB number as `0x800A0000 | number` and the description.
- **Threading.** Calls run on the interpreter's apartment thread, between
  statements of the serving loop.

## Known limits

No type library or `ITypeInfo` (early binding, `IDispatchEx`), no events from
served objects to their clients, no `Instancing` variants other than creatable
multi-use, no automatic reference-based termination of `Class_Terminate`, and
no in-process DLL servers (MP-0007). Per-user `LocalServer32` auto-start is
not exercised by the tests: on the development machine COM refused to start
per-user servers, so the process test starts the server itself.

## Verification

`TC-MP0005-com-server-inproc` and `TC-MP0005-com-server-process` (a server
process and a client process).

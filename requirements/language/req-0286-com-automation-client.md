# REQ-0286 — Late-bound COM Automation client

**Content type:** Project requirement

**Status:** Accepted (Windows)

## Statement

On Windows a program can create and drive real COM Automation objects through
`IDispatch`:

- `CreateObject(progid)` (a ProgID or `{CLSID}`) creates the server in or out
  of process. The emulated classes (`Scripting.Dictionary`,
  `Scripting.FileSystemObject`, `VBScript.RegExp`) take precedence over a
  registered server of the same ProgID. A missing or unregistered class raises
  error 429.
- `GetObject(path)` binds a file or moniker name (`winmgmts:`...) and
  `GetObject(, progid)` returns the running instance; failures raise 432
  or 429.
- On a COM object: `obj.Member`, `obj.Method(args)`, `obj.Method a, b`,
  `obj.Prop = v`, `Set obj.Prop = o`, `obj(args)` and `obj(args) = v` (the
  default member), named arguments (`name:=value`), omitted optional
  arguments, `With`, chained access, and `For Each` over a collection
  (`DISPID_NEWENUM`). A variable passed as an argument is passed by
  reference, so an `[out]` parameter updates the variable.
- Values marshal as follows: `Integer`/`Long`/`Byte`/`Single`/`Double`/
  `Currency`/`Boolean`/`Date`/`String`/`Empty`/`Null`/error values map to their
  Automation types; `Decimal` is sent as a Double; arrays map to
  `SAFEARRAY`s of Variant (multi-dimensional arrays keep their VB order);
  COM objects pass as `IDispatch`; `Nothing` is a null `IDispatch`. A VB class
  instance cannot be passed to a COM method (error 13).
- A server error raises the VB error with its number, `Source` and
  `Description` (and help file and context); `DISP_E_MEMBERNOTFOUND` is 438,
  `DISP_E_TYPEMISMATCH` 13, `DISP_E_BADPARAMCOUNT` 450,
  `DISP_E_PARAMNOTOPTIONAL` 449.
- **Events.** Assigning a COM object to a `WithEvents` field connects the
  object's default outgoing interface (found through `IProvideClassInfo`); the
  `field_Event` handlers run when the server raises the event. ByRef event
  arguments are written back to the server. Events from a server arrive while a
  COM call is running or during `DoEvents`, which pumps the thread's window
  messages. An error in a handler fails the COM call that was running.
- Two references to one server object compare equal with `Is`.
- `TypeName(obj)` is the coclass name when the server publishes
  `IProvideClassInfo`, else its dispatch interface name.
- COM is initialized as a single-threaded apartment on the interpreter thread.
  Other hosts raise 429 for real ProgIDs.

## Known limits

Early binding needs a project `Reference=` (`REQ-0288`); without one use
`As Object`. A VB class cannot implement a COM interface, and events of an
object without a type library cannot be received.

## Verification

`TC-MP0002-corpus-148-com-events` (an asynchronous `DOMDocument60` load
raising `onreadystatechange`) and `TC-MP0002-corpus-142-com-automation` (uses `WScript.Shell` and
`MSXML2.DOMDocument.6.0`, both shipped with Windows).

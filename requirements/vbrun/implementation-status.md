# VBRUN helpers — implementation status

**Content type:** Implementation record for `REQ-0100`–`REQ-0105`

**Reference:** installed `MSVBVM60.DLL` 6.00.9848 (x86), the file the inventory
`evidence/reference/msvbvm60-6.0-runtime.json` was taken from.

## Method

The reference runtime is on the development machine, so the helpers are
written from what it does and checked against it:

1. **Signature and behavior** come from the disassembly of the export
   (`dumpbin /disasm`; the stack size of `ret n`, the registers and stack slots
   the code reads, the imports it calls). Nothing is taken from a name alone:
   `rtcLeftBstr`, for example, counts bytes (it is `LeftB`).
2. **Probe.** `scripts/Probe-Vbrun.ps1 -Family <family>` calls the reference
   export from a 32-bit PowerShell with the same arguments; the expected values
   in `tests/vbrun/vbrun_helper_tests.cpp` are the output of that probe.
3. **Test.** `TC-MP0009-vbrun-helpers-<family>` calls the export of
   `wfcvbrun.dll` by name and checks the same results. It runs on every
   architecture of the matrix; the exports use `__stdcall` on x86, the only
   architecture whose binaries import them.

An export is listed in `src/vbrun/implemented.txt` when it has a real
implementation; `scripts/New-VbrunExports.ps1` then exports it under its own
name. Every other export stays a stub that stops the process with a message
naming it.

## Status by family

| Family | Requirement | Exports | Implemented | Notes |
| --- | --- | ---: | ---: | --- |
| Member and memory helpers | `REQ-0103` | 24 | 14 | `GetMem1/2/4/8`, `PutMem1/2/4/8`, `VarPtr`, `GetMemStr`, `PutMemStr`, `GetMemObj`, `SetMemObj`, `GetMemVar`. Not yet: `PutMemVar`, `PutMemObj`, `SetMemVar` (default-property calls through `IDispatch`), the `*NewObj` and `*Event` helpers (they call into the engine's object creation), `CopyRecord` (reads private record descriptors) |

(Updated with each batch.)

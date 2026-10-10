# REQ-0285 — Headless `App`, `Clipboard`, and `Screen` service objects

**Content type:** Project requirement

**Status:** Accepted

## Statement

Without forms or controls, the global service objects behave as follows. Each
is a predeclared instance created on first use unless the program declares its
own name.

- **`App`** (`REQ-0058`): every contracted property is present. `Title`,
  `HelpFile`, `TaskVisible`, the `OleServerBusy*` and `OleRequestPending*`
  properties are readable and writable, with the VB6 defaults (`TaskVisible`
  `True`, busy timeout 10000, pending timeout 5000, the stock message titles).
  `StartMode` is 0, `NonModalAllowed` is `True`, `UnattendedApp` and
  `RetainedProject` are `False`. `StartLogging` records `LogPath` and
  `LogMode`; `LogEvent` is accepted. `LegalTrademarks` comes from the
  project's `VersionLegalTrademarks`.
- **`Clipboard`** (`REQ-0052`): text only, held in the interpreter process and
  never the system clipboard, so running a program does not alter the user's
  clipboard. `Clear`, `SetText`, `GetText` and `GetFormat` (`vbCFText` and
  `vbCFRTF`-style text queries) work; `GetData`/`SetData` need pictures and
  belong to MP-0005.
- **`Screen`** (`REQ-0051`): `Width`, `Height`, `TwipsPerPixelX/Y`,
  `FontCount` and `Fonts(i)` come from the host display on Windows (1920 by
  1080 at 96 DPI elsewhere); `MousePointer` is stored; `ActiveForm` and
  `ActiveControl` are `Nothing` until forms exist (MP-0004).
- The `vbCF*` clipboard-format and `vb*` mouse-pointer constants are defined.

## Verification

`TC-MP0002-corpus-140-app-members` and `-141-clipboard-screen`.

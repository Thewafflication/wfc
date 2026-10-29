# MP-0010: Distribution, Libraries and Command Line

**Content type:** Milestone plan

**Status:** In progress (started on the owner's instruction, 2026-10-29)

## Goal

Make WFC installable and its libraries independent, versioned and linkable
either way, with a command line that explains itself.

## Scope and decisions

| Item | Decision |
| --- | --- |
| Issues | Issues 1 to 5 of `Thewafflication/wfc` fixed and closed (`corpus 200`). |
| Libraries | `lib/wfc-vbrun` (`wfcvbrun.dll`, `wfcvbrun_static.lib`) and one library per OCX: `lib/wfc-mscomctl` (`mscomctl.ocx`; `wfcmscomctl.dll`), `lib/wfc-comdlg32` (`comdlg32.ocx`; `wfccomdlg32.dll`) and `lib/wfc-mswinsck` (`mswinsck.ocx`; `wfcmswinsck.dll`, with the socket layer). Each is a self-contained directory: own CMake, `VERSION`, version resource, member table and the `.inc` behaviour the evaluator includes. The VB intrinsic control tables and the lookups stay in the core (`src/ui`). The four repositories exist under `Thewafflication`; replacing the directories with submodules (`scripts/Convert-LibrariesToSubmodules.ps1`, `git subtree split` keeps the history) is the next step. |
| Versioning | Every library has its own `VERSION` (0.9.0) stamped into the DLL; the install keeps `lib\<name>\<version>\<arch>`; the launcher takes the highest version and refuses a runtime of another major.minor. |
| Install | `scripts/Install-Wfc.ps1`, run by the package's `.wpm/install.cmd`: administrator installs to `C:\wfc` for all users, otherwise to `C:\wfc\users\<name>`. PATH, `WFC_HOME`, Start menu and Windows Terminal "WFC Command Prompt" (`bin\wfc-prompt.cmd`), Apps & features entry, `-Uninstall`. |
| Static or dynamic | Static (default): the program is a copy of `wfc.exe` plus payload. Dynamic: a copy of `wfcl.exe` plus payload, loading `wfcrt.dll`. `wfc build --link static|dynamic [--bundle]`. OCX libraries: `WFC_OCX_SHARED` builds each as its own DLL. Games are packaged both ways. |
| Command line | `src/cli.cpp`: `help`, `run`, `check`, `build`, `libs`, `env`, unknown-option suggestions, missing-file messages, `--link` validation, exit codes. `wfc build` now carries machine code by default (`--interpret` for the interpreter, `--native` to make a compile error fatal). |

## Tests

`TC-MP0010-cli-*` (help, mistakes, check, libs), `TC-MP0010-link-dynamic`,
`TC-MP0010-install` (layout, versioned libraries, a dynamic build with the
installed wfc), `TC-MP0010-dynamic-package-connect4` and `-pipes`,
`TC-MP0010-vbrun-static`.

## Work log

| Date | Entry |
| --- | --- |
| 2026-10-29 #1 | Issues 1 to 5 fixed and closed (Static records, `For ... Step 0`, `String` codes above 255, ByRef Variant write-back, a virtual 1920x1080 headless screen); corpus 200. |
| 2026-10-29 #2 | `lib/wfc-vbrun` and `lib/wfc-ocx` made self-contained (own CMake, `VERSION` 0.9.0, version resource); `wfc_vbrun_static`, `WFC_OCX_SHARED`. |
| 2026-10-29 #3 | CLI (`src/cli.cpp`), `wfcrt.dll` + `wfcl.exe`, `--link`/`--bundle`, `completion powershell`, version resources of the programs. |
| 2026-10-29 #4 | `scripts/Install-Wfc.ps1`, `.wpm/install.cmd` and `remove.cmd`, the WFC command prompt and terminal profiles; `wfc build` carries machine code by default; games packaged static and dynamic. |
| 2026-10-29 #5 | Performance: closing a form no longer repaints it or rebuilds its menu bar per child (Clix closes 0.15 s faster, load 0.78 s to 0.62 s); labels keep an unchanged alignment. |
| 2026-10-29 #6 | A crash in wfc reports itself (`src/crash.cpp`, `WFC_CRASH_STACK`); native code zeroes owned local slots at entry (a pending error freed garbage); GoTo onto a `Next`/`Loop`/`Wend` line. |
| 2026-10-29 #7 | Differential fuzzing of the interpreter against native code (a random-program generator; about 600 programs): block-header errors are trappable, skipped code raises nothing, Booleans order and select as numbers, `Str` of a Boolean, numeric strings, Empty and Null in `Select Case`; native `Int`/`Fix` of a Boolean. Corpus 201 to 204, native 74 and 75. |
| 2026-10-29 #8 | `wfc-ocx` split into `wfc-mscomctl`, `wfc-comdlg32` and `wfc-mswinsck` (one DLL each); the VB intrinsic tables moved to `src/ui`; the table types are one header, identical in the three libraries (a test checks it). |

## Open

- Submodule conversion of the four libraries below `lib/` (the repositories
  exist; `scripts/Convert-LibrariesToSubmodules.ps1 -Owner <owner>` splits the
  history out, pushes it and adds the submodules).
- Requirements 0142, 0153, 0155 and 0156 were revised for Boolean ordering and
  numeric strings in `Select Case` (they now follow Visual Basic); the owner
  should confirm.
- Left as they are because the Visual Basic answer is not known: `Select Case`
  of a number against `"x" To "z"` (the interpreter raises 13, native code goes
  to `Case Else`) and some Variant error numbers (13 against 11).
- The install script is tested for the file layout only (`-FilesOnly`); the PATH,
  Start menu and registry parts were not run on a real machine.

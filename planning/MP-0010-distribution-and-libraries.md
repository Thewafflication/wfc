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
| Libraries | `lib/wfc-vbrun` (`wfcvbrun.dll`, `wfcvbrun_static.lib`) and `lib/wfc-ocx` (control tables, Winsock layer, `comctl.inc`, `ocx.inc`) are self-contained directories: own CMake, `VERSION`, version resource, tests and generator scripts. Creating the two GitHub repositories and replacing the directories with submodules (`git subtree split` keeps the history) is held for the owner's approval; the auto-mode classifier refused creating public repositories. |
| Versioning | Every library has its own `VERSION` (0.9.0) stamped into the DLL; the install keeps `lib\<name>\<version>\<arch>`; the launcher takes the highest version and refuses a runtime of another major.minor. |
| Install | `scripts/Install-Wfc.ps1`, run by the package's `.wpm/install.cmd`: administrator installs to `C:\wfc` for all users, otherwise to `C:\wfc\users\<name>`. PATH, `WFC_HOME`, Start menu and Windows Terminal "WFC Command Prompt" (`bin\wfc-prompt.cmd`), Apps & features entry, `-Uninstall`. |
| Static or dynamic | Static (default): the program is a copy of `wfc.exe` plus payload. Dynamic: a copy of `wfcl.exe` plus payload, loading `wfcrt.dll`. `wfc build --link static|dynamic [--bundle]`. OCX tables: `WFC_OCX_SHARED` builds `wfcocx.dll`. Games are packaged both ways. |
| Command line | `src/cli.cpp`: `help`, `run`, `check`, `build`, `libs`, `env`, unknown-option suggestions, missing-file messages, `--link` validation, exit codes. `wfc build` now carries machine code by default (`--interpret` for the interpreter, `--native` to make a compile error fatal). |

## Tests

`TC-MP0010-cli-*` (help, mistakes, check, libs), `TC-MP0010-link-dynamic`,
`TC-MP0010-install` (layout, versioned libraries, a dynamic build with the
installed wfc), `TC-MP0010-dynamic-package-connect4` and `-pipes`,
`TC-MP0010-vbrun-static`.

## Open

- Submodule conversion of `lib/wfc-vbrun` and `lib/wfc-ocx` (needs the two
  repositories).
- The install script is tested for the file layout only (`-FilesOnly`); the PATH,
  Start menu and registry parts were not run on a real machine.

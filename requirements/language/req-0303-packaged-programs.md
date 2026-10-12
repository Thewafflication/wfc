# REQ-0303 — Packaged programs

**Content type:** Project requirement

**Status:** Accepted (implements `ADR-0007` decision 1)

## Statement

- **Building.** `wfc --build <project.vbp> -o <app.exe> [--gui]` writes an
  executable made of the running `wfc` image (the output has the architecture of
  the `wfc` that built it), a payload and a trailer (`WFCPKG01` and the payload
  size). The payload holds every file below the project's directory except the
  `.git` directory, the output itself and files ending `.exe`, `.dll`, `.pdb`,
  `.obj`; it is limited to 512 MB. `--gui` stores the option so the program
  starts in the Win32 host. Building from a packaged program drops the payload it
  carries.
- **Running.** At start-up `wfc` reads its own trailer. When it finds a payload
  it extracts the files below `wfc-pkg-<hash>` in the temporary directory
  (rewriting any file that is missing or differs, so a tampered copy is
  repaired), then runs the project exactly as `wfc <project.vbp> [options]`
  would. Command-line arguments of the packaged program are not forwarded.
- **Errors.** A damaged payload or an extraction failure is reported on stderr
  and exits with status 2; a path in the payload that is absolute or contains
  `..` is rejected.

## Known limits

The program is the interpreter plus the project text: there is no machine-code
generation. Files outside the project directory that the project names are not
included. A signed `wfc` loses its signature on the output.

## Verification

`TC-MP0007-package-*`: a packaged corpus project prints what the plain run
prints, including `Debug.Print` output.

# REQ-0126 — Drive, directory, and file list behavior

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to `DriveListBox`, `DirListBox`, and `FileListBox` on supported Windows targets.

## Requirement

`DriveListBox` shall enumerate reference-visible drives and update `Drive`; `DirListBox` shall normalize and enumerate `Path`; and `FileListBox` shall enumerate `Path` entries filtered by `Pattern` and the `Normal`, `ReadOnly`, `Archive`, `Hidden`, and `System` attributes. Their list, selection, scrolling, refresh, and change/path/pattern events shall match VB6. The controls shall not invent automatic cross-control synchronization beyond behavior observed when client event code assigns the related properties.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0126` VB6 reference and WFC conformance test

Use a controlled filesystem fixture and available drive types; test relative, absolute, root, UNC, missing, inaccessible, long, ANSI, and mixed-case paths; multiple patterns and attributes; refresh after mutation; selection/events; and explicit three-control synchronization.

The probe shall retain inputs, event traces, values, errors, and visual evidence
needed to distinguish the stated cases. WFC shall match the confirmed reference
result unless an approved tailoring decision states otherwise.

## Relationships

- **Derived from:** `evidence/reference/vb-6.0-type-library.json` and
  `evidence/reference/vb6-form-control-documentation.md`
- **Depends on:** Applicable VB class API contracts in `REQ-0037` through `REQ-0068`
- **Conflicts with:** None known

## Tailoring

A documented platform difference may be accepted only through an approved
compatibility-scope or architecture decision that identifies its user impact.

## Implementation Record

Not implemented.

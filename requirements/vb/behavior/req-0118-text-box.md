# REQ-0118 — TextBox editing and selection behavior

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to `TextBox` on supported Windows targets.

## Requirement

`TextBox` shall implement single-line and multiline editing, `Text` as its default value, `MaxLength`, `Locked`, `PasswordChar`, alignment, scrolling, selection through `SelStart`, `SelLength`, and `SelText`, and `HideSelection`. User and programmatic edits shall update text, caret, and selection and raise `Change`, keyboard, validation, and focus events in the VB6 cases and order. Invalid ranges, newline forms, clipboard commands, undo behavior, and native text limits shall match the reference profile.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0118` VB6 reference and WFC conformance test

Exercise typing, paste/cut/copy, selection replacement, programmatic Text and SelText changes, max length, password display, multiline newline and scrolling, locked/disabled states, focus loss, validation, Unicode/ANSI conversion, and invalid indices.

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

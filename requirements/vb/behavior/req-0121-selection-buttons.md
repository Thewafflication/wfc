# REQ-0121 — CheckBox and OptionButton selection behavior

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to `CheckBox` and `OptionButton` on supported Windows targets.

## Requirement

`CheckBox.Value` shall implement unchecked, checked, and grayed values where allowed, with user interaction cycling only through the reference states and raising `Click` as in VB6. Selecting an `OptionButton` shall set its `Value` true and clear the other option buttons in the same immediate container group without affecting other containers. Programmatic assignments, keyboard/mnemonic activation, alignment, graphical styles, pictures, masks, validation, and data binding shall match the reference.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0121` VB6 reference and WFC conformance test

Exercise every value by code and input, invalid values, click/event behavior, keyboard and mnemonics, multiple containers, initial multiple-true persistence, dynamic control arrays, graphical rendering, validation, and bound values.

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

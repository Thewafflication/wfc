# REQ-0127 — Menu hierarchy and command behavior

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to `Menu` and forms containing menus on supported Windows targets.

## Requirement

WFC shall reconstruct the design-time menu hierarchy, captions, separators, access keys, shortcuts, checked, enabled, visible, tag, help context, index/control-array, and `WindowList` behavior. Menus shall participate in form layout and MDI window listing as in VB6 and raise the correct `Click` handler for mouse, keyboard, mnemonic, shortcut, and programmatic paths supported by VB6. Run-time property changes shall update native presentation and command routing.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0127` VB6 reference and WFC conformance test

Exercise nested menus, separators, duplicate mnemonics, shortcuts, checked/disabled/hidden states, menu control arrays, popup menus, dynamic captions, right-to-left forms, MDI window lists, and every activation path.

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

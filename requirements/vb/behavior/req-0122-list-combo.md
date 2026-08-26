# REQ-0122 — ListBox and ComboBox item and selection behavior

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to `ListBox` and `ComboBox` on supported Windows targets.

## Requirement

The controls shall maintain zero-based `List` and `ItemData` entries, `ListCount`, `ListIndex`, `Text`, `NewIndex`, and `TopIndex`; implement `AddItem`, indexed insertion, `RemoveItem`, and `Clear`; and apply `Sorted`, columns, integral-height, and style rules. `ListBox` shall implement single, simple-multiple, extended-multiple, `Selected`, `SelCount`, and checkbox-item behavior. `ComboBox` shall implement simple, drop-down editable, and drop-down-list behavior including text selection, `Locked`, drop-down, change, click, scroll, and validation events.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0122` VB6 reference and WFC conformance test

Mutate empty and populated lists at all indices; test duplicates, sorting and NewIndex, item data, selection modes, keyboard/mouse range selection, checked items, top index, columns, every combo style, text editing, events, limits, and invalid indices.

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

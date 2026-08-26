# REQ-0117 — Label caption and mnemonic behavior

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to `Label` on supported Windows targets.

## Requirement

`Label` shall render its caption using the configured alignment, font, foreground/background, border, appearance, `BackStyle`, right-to-left, `AutoSize`, and `WordWrap` behavior. With `UseMnemonic` enabled, an ampersand mnemonic shall be displayed and route keyboard focus to the next eligible control according to VB6 tab order; escaped ampersands shall render literally. Caption changes shall update layout and raise `Change` as in VB6.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0117` VB6 reference and WFC conformance test

Compare raster output and geometry for empty, multiline, long, mnemonic, escaped-ampersand, right-to-left, transparent, autosized, wrapped, bordered, and disabled labels; trace Change and mnemonic focus.

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

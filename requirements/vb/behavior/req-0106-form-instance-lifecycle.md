# REQ-0106 — Form instance and load lifecycle

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to `Form` and `MDIForm` on supported Windows targets.

## Requirement

WFC shall create a form instance before its first executable event, raise `Initialize` once for each instance, load persisted controls and properties before `Load`, raise `Load` once per load cycle, and make the default form instance available through its class name. Referencing a property or invoking `Show` on an unloaded default instance shall load it. Re-entrant references during initialization or loading shall resolve to the same instance.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0106` VB6 reference and WFC conformance test

Record object identity and the ordered trace produced by explicit `Load`, implicit property access, `Show`, repeated `Load`, and a re-entrant reference from `Initialize` and `Load`.

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

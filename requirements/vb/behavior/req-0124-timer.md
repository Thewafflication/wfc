# REQ-0124 — Timer scheduling behavior

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to `Timer` on supported Windows targets.

## Requirement

An enabled `Timer` with a nonzero valid `Interval` shall raise `Timer` events on its owning UI thread according to VB6 message-driven scheduling. `Enabled = False` or `Interval = 0` shall suppress delivery. Changing interval or enabled state, including from within the handler, shall reschedule or stop future events as in VB6. Delivery may coalesce or drift under a blocked message loop and shall not recursively overlap a running handler unless observed in the reference.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0124` VB6 reference and WFC conformance test

Measure event timestamps and thread identity across boundary intervals, disabled/zero states, handler overruns, blocked message loops, modal forms, property changes inside handlers, unload, and system timer granularity.

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

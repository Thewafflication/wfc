# REQ-0130 — VB6 DDE link behavior

**Content type:** Project requirement

**Status:** Proposed

**Source:** Installed `VB6.OLB`; VB6 form/control documentation baseline

## Scope

This requirement applies to Forms and intrinsic controls exposing `LinkTopic`, `LinkItem`, `LinkMode`, or link methods/events on supported Windows targets.

## Requirement

Where the compatibility profile includes DDE, WFC shall implement manual and automatic link establishment, request, poke, execute, notification, timeout, close, error, and form-level command execution behavior with VB6 topic/item string rules and event cancellation. Where DDE is excluded, use of these members shall fail through one documented compatibility diagnostic rather than appearing to succeed.

## Rationale

The VB6 type library identifies this behavior as part of the form-runtime surface;
the details affect existing source, persisted forms, user interaction, or compiled
application behavior and therefore require an explicit compatibility contract.

## Verification

**Method:** Test

**References:** Planned `TC-0130` VB6 reference and WFC conformance test

Use a controlled DDE client/server pair to trace cold, warm, and hot links, timeouts, reconnects, request/poke/execute data conversion, event order, cancellation, server termination, malformed topics/items, and excluded-profile behavior.

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

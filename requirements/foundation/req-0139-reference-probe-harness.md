# REQ-0139 — VB6 reference-probe harness

**Content type:** Project requirement

**Status:** Proposed

**Source:** MP-0001 compatibility-evidence objective

## Requirement

WFC shall provide a repeatable harness that builds or runs controlled VB6
reference probes, captures source-visible results and environment identity in a
machine-readable form, and compares them with WFC results without treating
undocumented observations as universally normative behavior.

## Rationale

VB6 library metadata defines callable surfaces but not defaults, event order,
errors, persistence, locale effects, rendering, or other behavioral semantics.

## Verification

**Method:** Inspection and test

Run one deterministic probe against the recorded VB6 reference environment,
validate the captured record, run its WFC counterpart, and produce an explicit
match, approved variance, or failure result.

## Relationships

- **Derived from:** MP-0001 scope and verification stages
- **Depends on:** Reference environment record and REQ-0137
- **Conflicts with:** None known

## Tailoring

GUI, locale, timing, or machine-sensitive probes shall record and control the
relevant inputs. A single local observation shall be labeled accordingly and
shall not silently become a cross-version or cross-platform guarantee.

## Implementation Record

Not implemented. The installed reference environment and static discovery
evidence are recorded; executable probe dispatch remains to be created.

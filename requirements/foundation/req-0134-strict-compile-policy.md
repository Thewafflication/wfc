# REQ-0134 — Strict compile policy

**Content type:** Project requirement

**Status:** Proposed

**Source:** MP-0001 toolchain-quality objective

## Requirement

Every WFC-owned C++ production and test target shall compile as C++20 with a
target-scoped strict warning policy. MSVC targets shall use at least `/W4`,
`/WX`, `/permissive-`, `/Zc:__cplusplus`, and `/Zc:preprocessor`; supported
non-MSVC targets shall treat the project's selected comprehensive warning set
as errors.

## Rationale

A compiler/runtime project relies on precise language and ABI assumptions;
warnings must not silently become platform-specific behavior.

## Verification

**Method:** Inspection and test

Inspect target properties and compile commands, then build a small negative
fixture that is rejected by the active warning-as-error policy.

## Relationships

- **Derived from:** MP-0001 build stage
- **Depends on:** REQ-0133
- **Conflicts with:** None known

## Tailoring

Third-party targets are excluded. A warning suppression in WFC-owned code must
be narrow, documented at the use site, and justified by a compiler defect or a
required compatibility boundary.

## Implementation Record

The target-scoped policy is active and current MP-0001 targets compile cleanly.
The negative policy fixture remains to be added.

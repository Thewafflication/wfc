# REQ-0026 — ListView searching and sorting

**Content type:** Project requirement  
**Status:** Proposed  
**Source:** Microsoft Common Controls 6.0 installed help, `vbmthfinditem.htm`; `vbprosortedlistview.htm`; `vbprosortkey.htm`; `vbprosortorder.htm`

## Scope

This requirement applies to the `FindItem`, `Sorted`, `SortOrder`, and `SortKey` behaviors on supported Windows targets.

## Requirement

`FindItem` shall search text, subitems, or tags from the requested one-based index using whole-word-prefix or partial-prefix matching, and the ListView shall apply `SortOrder` and `SortKey` only while `Sorted` is true.

## Rationale

Search and sort semantics affect which object is returned and the observable ordering of every item.

## Verification

**Method:** Test

**References:** Planned `TC-0026` VB6 reference and WFC conformance test

The test shall exercise each stated condition against the installed reference
baseline and WFC, retain the observable results, and require equivalent results
unless an approved tailoring decision states otherwise.

## Relationships

- **Derived from:** The installed `cmctl198.chm` documentation and the relevant
  Automation contract requirement in `REQ-0001` through `REQ-0009`
- **Depends on:** Reference environment record and the applicable control API contract
- **Conflicts with:** None known

## Tailoring

A documented platform difference may be accepted only through an approved
compatibility-scope or architecture decision that identifies its user impact.

## Implementation Record

Not implemented.


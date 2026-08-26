# Visual Basic 6.0 Requirements

**Content type:** Project requirement index

**Status:** Proposed

This requirement set covers the Visual Basic 6.0 run-time object model exposed
by `VB6.OLB`. The reference library identifies the form surfaces, intrinsic
controls, drawing objects, application services, and their event contracts.

## Reference Baseline

| Item | Value |
| --- | --- |
| File | `C:\Program Files (x86)\Microsoft Visual Studio\VB98\VB6.OLB` |
| SHA-256 | `4109EE488C9E930F8884280526B4C36FFC7CEE03329CAC6A9B8F5864B15C8453` |
| Type library | `VB` 6.0 |
| Type-library GUID | `{FCFB3D2E-A0FA-1068-A738-08002B3371B5}` |
| Coclasses | 32 |

## Requirement Sets

- [VB class API contracts](classes/README.md) document every class, property,
  method, event, DISPID, and VB-visible type found in the library.

## Form-Class Boundary

The form-runtime architecture must account for these visual host classes:

- `Form` — ordinary top-level and MDI-child forms;
- `MDIForm` — MDI parent forms;
- `UserControl` — reusable hosted controls;
- `PropertyPage` — ActiveX property-editing surfaces; and
- `UserDocument` — ActiveX document surfaces.

`VB6EXT.OLB` exposes the separate `VBIDE` automation/extensibility model. It is
not required to represent or execute forms and will be documented as its own
profile if IDE compatibility enters scope.

Behavioral requirements will be derived from `VB98.CHM` and confirmed using
small compiled VB6 form probes, following the same documentation-to-evidence
process used for MSComctlLib.

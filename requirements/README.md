# WFC Requirements

**Content type:** Project requirement index

Project requirements use stable `REQ-NNNN` identifiers and follow the WSP
requirement format. Proposed requirements remain subject to review and may be
tailored only through controlled project decisions.

The [WFC 1.0 compatibility profile](../planning/compatibility-profile-1.0.md)
defines which requirements gate 1.0.0. A discovered feature may be removed or
deferred only through an explicit disposition; absence of an implementation is
not an implicit scope decision.

## Requirement Sets

- [0.1.0 foundation](foundation/README.md) contains the controlled dependency,
  build, architecture, vertical-slice, test-evidence, and reference-probe
  requirements that gate MP-0001.
- [Visual Basic 6.0 runtime](vbrun/README.md) contains the binary loader,
  execution-engine, compiler-helper, language-service, member-access, numeric,
  and export-layout contracts derived from the installed `MSVBVM60.DLL`.
- [Standard OLE 2.0](stdole/README.md) contains the font, picture, persistence
  function, native COM interface, and constant contracts derived from the
  installed `stdole2.tlb`.
- [Visual Basic for Applications 6.0](vba/README.md) contains the standard
  library class, function-module, enumeration, and constant contracts derived
  from `VBA6.DLL`.
- [Visual Basic 6.0 classes](vb/README.md) contains the run-time form,
  intrinsic-control, and service-class API contracts derived from `VB6.OLB`,
  plus form and intrinsic-control behavioral requirements.
- [MSComctlLib controls](controls/README.md) contains the Automation API
  contract requirements derived from the installed Microsoft Windows Common
  Controls 6.0 reference component.

The initial [control behavioral requirements](controls/behavior/README.md) are
derived from the installed Microsoft Common Controls help. Executable VB6
probes will confirm these requirements and establish additional defaults,
results, errors, persistence details, and undocumented interaction semantics.

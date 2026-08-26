# WFC — Waughtal Foundation Classes

A C++ reimplementation of the VB6 ecosystem:

* VB
* VBA
* VBRUN
* stdole
* MSComctlLib

The project targets Windows x86, x64, and ARM64 and uses WCRT and WSP as its
runtime and engineering-process foundations.

## Planning

The initial work plan is [MP-0001 — Foundation and Feasibility](planning/MP-0001-foundation-and-feasibility.md).
The [VB6 discovery baseline](planning/reference-environment.md) records the
installed reference components and inspection tools.

The [project requirements index](requirements/README.md) contains the proposed
compatibility contracts produced by discovery.

The [Visual Basic class requirements](requirements/vb/README.md) identify the
form hosts, intrinsic controls, and run-time service classes exposed by VB6.

The [VBA requirements](requirements/vba/README.md) inventory the standard
library classes, function modules, enumerations, and constants.

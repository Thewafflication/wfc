# VB6 Form and Intrinsic-Control Documentation Baseline

**Recorded:** 2026-08-26

The installed Visual Basic 6.0 environment does not contain `VB98.CHM` or
another local copy of the core VB6 language/control help. A recursive search of
the installed `VB98` directory found only `SCCVBUS.CHM`, which documents source
control integration and is not relevant to form-runtime behavior.

The behavioral requirements therefore use these sources in descending order
of authority:

1. the installed `VB6.OLB` type library and its retained machine-readable
   inventory, which establish the available form/control properties, methods,
   events, parameter types, and event cancellation arguments;
2. executable probes compiled with the installed VB6 IDE, which are the planned
   acceptance authority for defaults, ordering, errors, rendering, focus,
   timing, persistence, and other behavior not encoded by the type library;
3. preserved Microsoft documentation that describes equivalent VB6 or VBA
   form concepts, used only to seed probeable requirements:
   - <https://learn.microsoft.com/en-us/dotnet/api/microsoft.visualbasic.compatibility.vb6.basecontrolarray?view=netframework-4.8.1>
   - <https://learn.microsoft.com/en-us/dotnet/api/microsoft.visualbasic.compatibility.vb6.basecontrolarray.load?view=netframework-4.8.1>
   - <https://learn.microsoft.com/en-us/dotnet/api/microsoft.visualbasic.compatibility.vb6.basecontrolarray.unload?view=netframework-4.8.1>
   - <https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/activate-deactivate-events>
   - <https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/groupname-property>

The supplemental pages are not assumed to be identical to the VB6 intrinsic
runtime. Every resulting claim remains Proposed until its named VB6 reference
test records the behavior of the installed VB6 6.0 baseline. If original VB6
help media becomes available, its file identity and relevant topic identifiers
shall be added here and the requirements reviewed against it.

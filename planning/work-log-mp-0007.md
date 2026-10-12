# Work Log — MP-0007 Native Compiler and VBRUN ABI

**Content type:** Work log (per `wsp/processes/work-log-template.md`)

**Milestone or work package:** MP-0007 — see [the plan](MP-0007-native-compiler.md).

**Period:** 2026-10-12 onward.

**Starting baseline:** MP-0006 closeout.

**Author:** Claude (Overlord cross-project assistant), on behalf of the owner.

| Date / # | Type | Work | Verification |
| --- | --- | --- | --- |
| 2026-10-12 #1 | Planning | MP-0006 closed; `ADR-0007`, MP-0007 plan and work log drafted | Not applicable |
| 2026-10-12 #2 | Construction | `wfc --build` packaged programs (`REQ-0303`, `ADR-0007`): image copy plus project payload, extraction on start-up; `TC-MP0007-package-*` | CTest 364/364 |
| 2026-10-12 #3 | Construction | `wfcvbrun.dll`: export directory generated from the MSVBVM60 inventory (`REQ-0105`), stubs that name the reached export (`REQ-0099`); `TC-MP0007-vbrun-library` parses the built PE | CTest 365/365 |
| 2026-10-12 #4 | Closeout | README packaging section; proposed closeout record | Not applicable |
| 2026-10-12 #5 | Closeout | MP-0007 closed; roadmap split: MP-0008 project compatibility, MP-0009 native compilation, closure candidate now MP-0010 (0.10.0), 1.0.0 now MP-0011 | Not applicable |

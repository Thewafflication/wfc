# MP-0009 — Native Compilation: Work Log

Plan: [MP-0009](MP-0009-native-compilation.md). Decision record:
[ADR-0008](../architecture/adr-0008-native-compilation-pipeline.md).

Goal-level token usage and elapsed time are `Not reported` (no telemetry is
available to the agent).

| Date / # | Type | Work | Verification |
| --- | --- | --- | --- |
| 2026-10-20 #1 | Planning | ADR-0008: a separate compiler front end (lexer, parser, AST, IR) with one back end per architecture, a shared C++ runtime reached through an entry table, a native payload in the packaged program, and the interpreter as the oracle for every native test | Documents |
| 2026-10-20 #2 | Construction | First vertical slice on AMD64 (`src/compiler/`): lexer, parser (modules, Sub/Function, `Dim`, assignment, `Print`, `If`/`ElseIf`/`Else`, `For`, `Do`/`Loop`, `While`, `Exit`, `End`, calls with ByVal and ByRef), lowering to a slot-based three-address IR with Integer and Long overflow checks, division by zero, an x64 emitter, a loader (`wfc --native <project>`: compile in memory and run) and a runtime entry table of six services. Eight programs (`tests/native`) produce the same output and exit code as the interpreter, including error 6 and error 11 | CTest 416/416 |

# WFC Repository Instructions

## Owner Authorization

- Ordinary file edits within this repository and the current task scope do not
  require asking the owner for approval each time.
- After each coherent work set is implemented and locally verified, commit it
  and push it to `origin/master` automatically. Do not leave completed work
  uncommitted or unpushed.
- Run only the x64 configure, build, and test suite locally. Do not run local
  x86 builds/tests or ARM64 cross-builds; GitHub Actions validates those targets
  after commit and push. The owner monitors routine Actions runs, so do not wait
  unless a known failure needs investigation and repair.
- Running the local build and test tooling is pre-authorized and does not
  require asking the owner. This includes `cmake` (configure and build), `ctest`,
  and running the produced `wfc` binaries for the x64 debug preset. Do not pause
  to request approval before invoking these; just run them and report results.
- Land every change the same way: stage the named paths and run plain
  `git commit`, then `git push origin <branch>:master`. The pre-commit hook
  is the gate (lint, x64 debug build, full test suite); never run those
  stages by hand in place of it, and never skip it with `--no-verify`. Its
  first step opens the always-on-top test progress window
  (`scripts/Show-TestProgress.ps1`), so the owner can watch the run; do not
  suppress or replace it.
- Only one commit gate may run at a time (`out/commit-gate.lock`, taken by
  `scripts/Start-CommitGate.ps1`; the progress window is likewise a single
  instance). Do not start a second `git commit`, build or `ctest` run in the
  same checkout while a gate is running, and do not edit sources until it
  ends. If a commit is refused because a gate is running, wait for it.
- The preferred Codex launch posture for this repository is workspace write
  access with on-request approvals:

  ```powershell
  codex -C C:\Users\jmwau\source\repos\wfc -s workspace-write -a on-request
  ```

- Platform-enforced approval prompts must still be honored. Destructive actions
  and actions outside the repository or current task scope require their normal
  explicit authorization.
- The owner monitors routine remote GitHub Actions runs. Do not wait for them
  after a push unless a known failure needs investigation and repair.

## Work Logs

- When maintaining a WSP work log, include goal-level token usage and elapsed
  time from the Codex goal-completion report. Label unavailable telemetry as
  `Not reported`; do not estimate it.

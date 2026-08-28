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

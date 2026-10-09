# Commit Checks

**Content type:** Project check inventory and developer setup

**Status:** Active since 2026-10-09 (WSP 1.4.0 `WSP-CHECK-0001`–`0008`)

## Canonical commands

The same three scripts run in the pre-commit hook, in CI, and by hand:

| Stage | Command | Scope |
| --- | --- | --- |
| Lint | `pwsh -NoProfile -File scripts/Invoke-Lint.ps1` | Complete owned source and configuration (below) |
| Build | `pwsh -NoProfile -File scripts/Invoke-Build.ps1 [-Preset <name>]` | Configure and build one preset; default `windows-x64-debug` |
| Test | `pwsh -NoProfile -File scripts/Invoke-Tests.ps1 [-Preset <name>]` | Every registered CTest test for the preset, including negative tests; zero tests is an error |

The local gate builds and tests `windows-x64-debug` only. CI runs lint once
and build/test for `windows-x86-debug`, `windows-x64-debug`, and
`windows-arm64-debug` (`WSP-TEST-0013`). Each script stops at the first
failure and exits nonzero (`WSP-CHECK-0004`). Lint changes no files.

## Setup

Prerequisites: Visual Studio 2026 (or 2022 for the x86/x64 presets) with the
C++ workload and Windows SDK, CMake 3.28 or newer, PowerShell 7, Python 3.12,
and Git with submodules.

```powershell
git submodule update --init wsp wcrt
python -m pip install --user --requirement requirements-dev.txt
Install-Module PSScriptAnalyzer -RequiredVersion 1.25.0 -Scope CurrentUser
pre-commit install
pre-commit run --all-files
```

`requirements-dev.txt` pins the Python-distributed tools. The scripts verify
each tool's version before use and fail when it is missing or different
(`WSP-CHECK-0007`); hooks never install tools or modify sources.

| Tool | Version | Source |
| --- | --- | --- |
| pre-commit | 4.6.2 | `requirements-dev.txt` |
| clang-format | 22.1.3 | `requirements-dev.txt` |
| gersemi | 0.29.2 | `requirements-dev.txt` |
| PSScriptAnalyzer | 1.25.0 | PowerShell Gallery |
| Doxygen | 1.18.0 | Not yet used by a check (documentation gap) |

Build output goes to `out/` (ignored). The hook runs once per commit, even for
documentation-only commits (`always_run`, `pass_filenames: false`,
`require_serial`), so a commit takes about a minute with a warm build.

## Check inventory (`WSP-CHECK-0006`)

| WSP requirement | Check | Configuration | Notes |
| --- | --- | --- | --- |
| `WSP-STYLE-0001`–`0003`, `0007` | `wsp/tools/Test-SourceStyle.ps1` in lint | Scope list in `scripts/Invoke-Lint.ps1` | Missing inputs and an empty scan fail |
| `WSP-STYLE-0004`, `WSP-LANG-0002` | `clang-format --dry-run --Werror` in lint | `.clang-format` | All `.cpp`/`.hpp` under `src`, `include`, `tests` |
| `WSP-LANG-0010` | `gersemi --check --warnings-as-errors` in lint | `.gersemirc` | Unknown CMake commands are findings |
| `WSP-LANG-0007` | WSP `Test-RepositorySyntax.ps1` and PSScriptAnalyzer in lint | `PSScriptAnalyzerSettings.psd1` | Analysis covers `scripts/` |
| `WSP-LANG-0004`, `WSP-LANG-0005` | YAML and JSON parse in lint | — | Workflows, hook config, `CMakePresets.json` |
| `WSP-CHECK-0001` | `pre-commit validate-config` in lint | `.pre-commit-config.yaml` | |
| `WSP-TEST-0001`–`0005` | `scripts/Invoke-Tests.ps1` | `CMakeLists.txt` | Includes `TC-MP0002-diagnostic-catalog` |
| `WSP-CHECK-0008`, `WSP-TEST-0012` | `.github/workflows/build.yml` | Lint job plus build/test matrix | Uses the same scripts |
| `WSP-STYLE-0006`, `WSP-LANG-0002` documentation | **Gap** | — | Doxygen contracts not yet written or checked |
| `WSP-SAST-0001`–`0006` | **Gap** | — | clang-tidy not yet configured |
| `WSP-SEC-0015`–`0016` | `TC-WSP-SEC-0016-pe-hardening-*` in `scripts/Invoke-Tests.ps1` | `wsp_enable_hardening()` in `CMakeLists.txt` | Final images must show ASLR, NX, CFG (and high-entropy VA on 64-bit); a negative test rejects an unhardened fixture |

Gaps are recorded in the WSP adoption record and are not reported as passing.

## Owned scope and exclusions

Lint covers `src/`, `include/`, `tests/`, `cmake/`, `scripts/`, `.github/`,
and the root build and configuration files. The exclusions (submodules,
build output, generated evidence, VB6 test fixtures, and the retired
`tools/` discovery scripts) are listed in the adoption record's
"Owned-source exclusions".

## Expected failures (`WSP-CHECK-0005`)

None. No test is allowed to fail; negative tests assert their expected
diagnostic and report Pass.

## Negative verification

On 2026-10-09 each lint check was shown to fail on a planted defect and pass
again after reverting it: an 81+ character line, misformatted C++,
misformatted CMake, a PSScriptAnalyzer finding, a YAML syntax error, and an
unknown CMake command. A failing CTest run (`-Preset no-such-preset`)
propagated exit code 1.

## Emergency bypass

`git commit --no-verify` skips the hooks. CI still runs the full checks and a
red CI run blocks acceptance (`WSP-CHECK-0008`); a bypass is recorded in the
work log with its reason.

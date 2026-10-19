# MP-0008 — WPM Package and First Release Plan

**Content type:** Release plan (draft)  
**Status:** Draft; nothing is tagged, signed or published until the owner
approves the exact set  
**Milestone:** [MP-0008](MP-0008-project-compatibility.md), stage 6

## Goal

After the MP-0008 gate, publish the first WFC release: the `wfc` tool and
runtime for x86, x64 and ARM64, and the demonstration programs built from the
`vb6games` corpus, as WPM packages. The process copies the one that WCRT uses
(`wcrt/docs/release-process.md`, `wcrt/tools/build-wpm-package.ps1`).

## Packages

| Package | Contents | Notes |
| --- | --- | --- |
| `wfc-any-<version>.zip` | `x86`, `x64`, `arm64` directories each with `wfc.exe` and `wfcvbrun.dll`; `LICENSE`; `README.MD`; the compatibility record | One package for all architectures, `arch=any`, license `GPL-3.0-or-later` |
| `wfc-games-any-<version>.zip` | For each corpus game and architecture a packaged program (`wfc --build`), the game's `PROVENANCE.md` and license, and the corresponding source revision of `vb6games` | The packaged programs embed the runtime, so they run without `wfc` installed. Each game's author license and credit travel with it |

## Build and trust order

1. Freeze and commit a clean source revision; record the Git revision, the
   `wcrt`, `wsp` and `vb6games` submodule revisions.
2. Run the Debug matrix (x86, x64, ARM64) green, including the corpus
   acceptance tests.
3. Build Release on all three architectures; run the same corpus tests against
   the Release binaries.
4. Build the games with `wfc --build` per architecture and verify that each
   program starts and ends (`--exit-after`), shows its forms (screenshots), and
   carries its version information and icon.
5. Assemble the packages with `wpm build`, sign them with the WPM release key,
   `wpm verify` them, and hash them (`SHA256SUMS`).
6. Write the release-readiness record with the exact artifact digests and the
   known limitations.
7. Only with the owner's approval of that exact set: tag, create the GitHub
   release, upload.

## Gaps before this can run

| Gap | Owner action or work |
| --- | --- |
| The CI workflow had Debug only. A Release matrix now exists (`windows-<arch>-release` presets and a `Release` job that builds, runs the whole suite and records artifact digests); a package job and a publish job do not exist | Add them, modelled on WCRT's workflow, fail-closed |
| A WPM signing key | Owner creates it with `wpm keygen`, commits `release_keys/wfc-release.public` and stores the private half as the secret `WPM_RELEASE_PRIVATE_KEY` of the protected `release` environment (steps in `release_keys/README.md`); the WCRT key is separate. The CI `package` job (version tags only) reads the secret, signs both packages and verifies them against the public key; it fails closed when either is missing |
| Dependency on `wcrt` | The package records the exact `wcrt` revision (ADR-0002) |
| Version number | `0.8.0` at the MP-0008 closeout, owner confirms |
| Licenses of any further corpus games | Checked per game before it is packaged |

Done so far: `scripts/Build-WpmPackage.ps1` (builds `wfc` and `wfc-games` from Release builds, unsigned unless a key is given; `wpm verify` refuses unsigned packages, as expected), the Release matrix and the packaged-program test of every corpus game (`TC-MP0008-package-*`). The CI `package` job exists (signs with the environment secret, verifies, uploads the packages and their checksums). Not done: the key itself, and publishing, which stays a separate step that needs the owner's approval of the exact files. The process needs the owner's decisions on the key,
the version, and the publish step.

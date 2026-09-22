# Release Runbook

How a pyCircuit release is produced, what has to be true before dispatch, and
what to do when a lane fails. `docs/development/sdk-release-contract.md` is the
normative contract; this page is the operational order of operations.

## Release identity

A release is one annotated tag `v<version>` plus the GitHub Release it points
at. The version must agree everywhere before dispatch:

| Fact | Source |
| --- | --- |
| `project.version` | `pyproject.toml` |
| candidate tag | `packaging/sdk/version-map.json` |
| tag does not exist yet | `git ls-remote --tags origin refs/tags/v<version>` |

The release publishes, in one asset set:

- `pycircuit-sdk-<version>-{linux-x86_64,macos-arm64,windows-x86_64}.tar.gz`
  plus an attached `.manifest.json` and `.lock.json` per platform,
- exactly one `pycircuit_hisi` wheel per platform (`linux_x86_64`,
  `macosx_*_arm64`, `win_amd64`). That wheel carries both frontends
  (`pycircuit`, `agentic_circuit`, `_pycircuit_semantics`) and both compilers
  (`pycc`, `acc`), so no consumer needs a second distribution,
- `pycircuit-sdk-<version>-release-index.json`, `LICENSES.tar.gz`, release notes,
- one `pyc-tools-<platform>:v<version>` GHCR artifact per platform.

PyPI publication is **not** part of `release.yml`. The package host is outside
the language contract, so it runs as the separate `Publish accepted wheels to
PyPI` workflow (`.github/workflows/publish-pypi.yml`) over bytes that the release
already published. A rejected upload therefore cannot invalidate a release or
its attestation, and an already cut release can still be published later without
rebuilding, re-tagging, or moving the source revision pin:

```bash
gh workflow run publish-pypi.yml --repo PTO-ISA/pyCircuit \
  -f version=6.1.0 -f source_revision=<accepted revision>
```

The workflow refuses to run unless the release is published, non-prerelease, and
its tag peels to the given source revision. It then selects exactly the wheels
that carry the release version (one platform wheel per supported platform),
checks each one's recorded size and its `METADATA` name and version, reports
every wheel it skips, and uploads the result.

Enabling it needs the release itself to be green, so the tag exists and the
attestation is linked, plus **one** of these two account-side setups:

1. a PyPI trusted publisher for the single shipped project `pycircuit-hisi`
   (owner `PTO-ISA`, repository `pyCircuit`, workflow `publish-pypi.yml`,
   environment `release`; the job requests `id-token: write` and uses
   `environment: release`), or
2. a `PYPI_API_TOKEN` repository secret holding a project-scoped PyPI API token,
   which the publish step uses instead of the OIDC exchange.

Creating a pending publisher is a PyPI web-console action (there is no API for
it), so it cannot be automated from this repository.

PyPI refuses a version that already exists, so a package already uploaded there
can only be superseded by a new version. Because of that the upload runs with
`skip-existing`, so a partially completed upload is finished by re-dispatching
the same workflow instead of becoming permanently unpublishable; the skipped
files are printed in the job summary.

## Entry criteria

Every gate below is part of the release job, but the cheap ones are worth
running before spending an hour of CI:

```bash
python3 flows/tools/check_decision_status.py \
  --rfc docs/rfcs/pyc6-decisions.md \
  --status docs/gates/decision_status_v6.md \
  --out /tmp/status.json \
  --require-no-deferred --require-all-verified \
  --require-concrete-evidence --require-existing-evidence
python3 tools/pycircuit/check-pyc-inventory.py
python3 .github/scripts/validate_repo_management.py
python3 -m pytest tests/unit -m unit
SKIP=pyc-api-hygiene pre-commit run --all-files
mkdocs build --strict
```

Two traps are worth naming, because both pass locally and fail in CI:

- **Evidence must be tracked by git.** `docs/gates/logs/*` is ignored, so a new
  attestation needs `git add -f`. `--require-existing-evidence` resolves paths
  on the checked-out tree, and the release lane checks out the candidate SHA.
  `tests/unit/test_release_evidence_and_platform_constraints.py` enforces this.
- **Hooks and tracked-file tests only see staged files.** `pre-commit run
  --all-files` skips untracked paths, and so do the unit tests that enumerate
  `git ls-files` (documentation navigation, repository layout). Stage new files
  before running the local gates; a brand-new page or test otherwise reports a
  failure that disappears once it is committed.

## Pipeline

`.github/workflows/release.yml` is dispatched with `version` and `commit_sha`;
every job checks out `commit_sha`, and no job rebuilds an earlier job's bytes.

| Stage | What it gates |
| --- | --- |
| `full-validation` | Integrated toolchain build, full Agentic Circuit closure (`check-acir` + `check-pyc` + ctest + e2e), repository contracts (pre-commit, unit tests, API hygiene, decision status, `mkdocs --strict`), full pyCircuit closure (examples, sims, nightly sims, semantic regressions). Nothing else starts until this passes. |
| `build-platform-candidates` | One retained candidate per platform, in parallel. The Windows lane provisions the pinned clang-cl 22.1.8 driver and builds MLIR from the pinned LLVM source (the LLVM Windows release ships no MLIR). |
| `aggregate-candidate` | `LICENSES.tar.gz` and `RELEASE_NOTES.md`, then deterministic aggregation into one candidate set, plus the SDK contract check over the release index and all three platform manifests/locks. |
| `verify-platform-candidates` | Per platform: relocation, manifest closure, native dependency closure (`dumpbin` on Windows), wheel set, and installed smoke. |
| `accept-candidate` | Acceptance barrier; the accepted bytes are what gets tagged. |
| `create-tag` | Annotated tag after acceptance, verified against `commit_sha`. |
| `publish-release`, `publish-ghcr` | GitHub Release and the three GHCR archives. |
| `verify-published-bytes` | Re-downloads the release URLs and compares them with the accepted bytes. |
| `verify-stable-platforms` | Re-verifies the published bytes on each real platform. |
| `release-attestation` | Requires all three platform attestations plus the publication attestation, writes the final attestation artifact, then links it from the release notes and issue #61. |

A failed `full-validation` stops the run before anything is tagged or
published, so a dispatch is safe to repeat.

## Windows

Windows is a first-class platform profile, not an afterthought:

- profile `windows-x86_64`: `windows-2022`, `x86_64-pc-windows-msvc`, MSVC v143
  ABI, Python 3.11, one wheel `win_amd64`. Verilator is not available there, so
  Verilog simulation stays on Linux/macOS.
- the compiler driver is the pinned clang-cl 22.1.8 from the official LLVM
  Windows release, over the MSVC v143 headers/libraries/linker. `cl.exe` cannot
  build the tree (C1001 on the recursive generic lambdas in ACIR codegen).
- the decision row for the profile must already be `implemented-verified` before
  a release is dispatched, because `full-validation` requires every decision to
  be verified. That is an ordering deadlock for a new profile, which is why
  `.github/workflows/platform-evidence.yml` exists: it builds and verifies one
  platform candidate with exactly the release commands and never publishes.

Bring-up order for a new profile:

1. dispatch `platform-evidence.yml` for the profile at the candidate revision,
2. commit its attestation and a summary under `docs/gates/logs/<date>-<profile>/`
   (with `git add -f`),
3. point the decision status row at those paths and mark it
   `implemented-verified`,
4. dispatch the release.

Portability traps already paid for, each with a regression test:

| Symptom | Cause | Fix |
| --- | --- | --- |
| `unknown type name 'DWORD'` in `psapi.h` | `psapi.h` included before `windows.h` | include `windows.h` first |
| `closeFile(file_t &)` does not compile | `file_t` is a HANDLE on Windows | close the reserved file through `llvm::raw_fd_ostream` |
| `cannot publish generated bundle` | `fs::rename` cannot move a directory on Windows | `acir::publishDirectory` (`MoveFileExW`) |
| venv interpreter not found (`WinError 2`) | `TEMP` is an 8.3 short path (`RUNNER~1`) | resolve the workspace to its long spelling first |
| `acc.py.exe` missing | pip does not materialise a launcher for an entry point whose name carries a suffix | prefer the launcher, otherwise run the module through the venv interpreter |
| `Agentic Circuit native extension is unavailable` | a wheel was installed that does not carry the native bridge (for example a hand-built pure-Python package) | install the published platform wheel, which carries `agentic_circuit/_native` |
| `pycc` / `acc` starts from the SDK tree but not from the installed wheel | the wheel was built without relocation, so it still references the builder's absolute library paths | rebuild with `create_wheel.py --platform <profile>`; the verifier names all three copies and whether they are byte-identical |
| an installed compiler aborts once immediately after extraction (macOS signal 6, Windows `0xC0000005`) | the freshly written binary was not yet fully available to the loader; observed once on windows-2022 | re-run the verification; the failure report distinguishes a flake from a defect by naming the console script, the wheel binary, and the SDK tree binary |
| `output AC unit must not already exist` | the driver refuses to clobber artifacts | regenerate into a second path and compare bytes |

## Diagnostics

- `closure-probe.yml` (dispatch-only) runs the closure steps independently with
  `continue-on-error`, so one run reports every step's outcome before anything is
  published. Its `Report closure outcomes` step prints the real table; the jobs
  API only shows the masked `conclusion`.
- Gate logs are uploaded as `release-gate-logs-<run-id>-<attempt>`; platform
  candidates as `platform-evidence-bytes-<platform>` and
  `platform-evidence-attestation-<platform>`.
- The Windows lane is the slowest cold path (LLVM/MLIR from source) but is
  cached per pinned source hash, so a repeat dispatch is minutes, not hours.

## Parallelism

| Layer | Control |
| --- | --- |
| CI jobs | platform matrix, three lanes at a time |
| Native toolchain build (`flows/scripts/pyc`) | `ninja` default (`#cores + 2`); no explicit cap |
| AC closure (`run_agentic_circuit.sh`) | `-j ${PYC_BUILD_JOBS:-6}`, `ctest -j ${PYC_TEST_JOBS:-6}` |
| `pyc build --jobs` | `max(1, os.cpu_count())` by default; flow scripts pin 4 (`PYC_EXAMPLE_JOBS`, `PYC_SIM_JOBS`) |
| Backend jobs | per-module `pycc` in a `ProcessPoolExecutor`, submitted in sorted order so retained bytes stay deterministic |

Anything that lists paths for a manifest must sort with an explicit key; a bare
`sorted()` over `glob`/`rglob`/`iterdir` is rejected by
`test_packaging_path_sorts_pass_an_explicit_key`. Never pipe a Python producer
into a consumer that exits early (`awk ... exit`, `head`): the producer raises
`BrokenPipeError` at shutdown, exits 120, and `pipefail` fails the gate.

## After the release

- The tag and its assets are immutable. Fix forward with the next patch version;
  the workflow refuses to run for an existing tag.
- Re-verification of the published bytes happens inside the same run
  (`verify-published-bytes`, `verify-stable-platforms`). Re-run those jobs, not
  the whole release, if a later check flakes.
- `docs/gates/logs/` accumulates one directory per gate run; keep the accepted
  release attestation, and treat the rest as disposable evidence.

## Re-releasing a version

The tag is the release pin, so re-cutting a published version rewrites its
identity: `v<version>` starts pointing at a different revision. Only do it while
no consumer has pinned the old revision, and only when the published version is
unusable (for example the release index or consumer lock is missing what the
consumption flow needs).

```bash
# 1. remove the release and its tag
gh release delete v<version> --repo PTO-ISA/pyCircuit --yes --cleanup-tag

# 2. make sure the version metadata still names the version being re-cut
#    (pyproject.toml and packaging/sdk/version-map.json candidate_tag)

# 3. preflight the revision that will carry it, then publish once
gh workflow run closure-probe.yml -f commit_sha=$SHA
gh workflow run platform-evidence.yml -f version=<version> -f commit_sha=$SHA -f platform=all
gh workflow run release.yml -f version=<version> -f commit_sha=$SHA
```

`release.yml` validates `! git ls-remote --exit-code --tags origin
refs/tags/v<version>`, so step 1 has to complete first, and the new tag is
created only after the candidate is accepted.

If a version already exists on PyPI it cannot be re-uploaded; cut the next patch
version instead.

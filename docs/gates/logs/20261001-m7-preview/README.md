# M7-01 local preview acceptance evidence

Accepted: 2026-10-01 for macOS 26.6.2 arm64. Source baseline:
`25152f9c630de6d7d019edcd6c9ed09ce1bebbfc`.
Reviewed fourteen-file dirty overlay:
`8b5e7c857bae3abad72e1d2495e9e21f19dbe0d809672756d0f794368b105250`.
`candidate-manifest.json` records the exact bytes; native/prefix/wheel metadata
binds baseline HEAD and the manifest binds this build/gate/docs-only correction.
No public language/IR/CLI/schema/ABI/manifest/version contract changed.

| Gate | Actual outcome | Evidence |
| --- | --- | --- |
| Fresh current-checkout configure/build/install | exit 0, 197 native build steps | build.txt; environment.json |
| Corrected actual build preset | exit 0, 112 steps | preset-build.txt |
| Native ACIR tests | 20/20, exit 0 | native.txt |
| Semantic R3, 26 fixture files | 311 passed, 2 V44 cases deselected, exit 0 | semantic-final.txt; semantic_regressions.stdout; semantic_regressions_summary.json |
| Unit suite, unit marker | 324 passed, 3 Windows-only skips, 79 deselected | unit.txt; unit.xml; unit-skips.json |
| Independent preset/retirement/layout | 19 passed | independent-cmake-presets.log |
| Independent public/Runtime/CompilerDev/model ABI | 25 passed | independent-public-runtime-smoke.log |
| Independent moved compiler prefix | 1 passed | independent-installed-relocation.log |
| Disposable local wheel installation | 1 passed | independent-wheel-smoke.log; local-wheel-binding.json |
| Source/install retirement | passed | retirement.txt |
| Repository management | passed | repository-management.txt |
| Strict decision status | 283 rows, zero deferred, passed | decision-status.txt; decision-status.json |
| All-files hooks and strict MkDocs | passed | precommit.txt; docs.txt |

Decision-status is structural registry/evidence validation. Earlier decision rows
retain historical evidence and are governed by Decision 0283; this check does
not prove all deferred capability backlog implemented. Semantic summary's
`tests: 26` denotes fixture files; the pytest count is 311, not 26.

## Current-candidate construction

All native code was freshly compiled in `.pycircuit_out/m7-01-root`, installed
into `.pycircuit_out/m7-01-install` and tested from this checkout. No copied
compiler/runtime from another worktree or old native cache was accepted.
The four current preset flags are ON; targets are the source-unit, design and
CPP source-parts helpers plus `pyc6_runtime`. Independent preset configuration
uses an isolated `-B` directory and tests target resolution/build against the
fresh PM tree. A separate real `--build --preset release-tools` run proves the
fixed default preset, while the old preset failed on its removed `pycc` target.

The existing semantic script includes all earlier fixture groups, the four
M6 system files and the new preset case. It therefore covers the example/sim/
nightly fixture sets without redundant test runs; the four formal release script
commands were not all dispatched separately. Stable release workflow protection
is unchanged. Two explicitly named V44 scheduling/reordering cases remain
excluded; build `-j4` does not establish simulation parallelism.

The local wheel is
`pycircuit_hisi-6.1.0-py3-none-macosx_26_0_arm64.whl`, SHA-256
`9707a2dbbf05960ae574f4905a364b86f948a6a654a33b827f3f604909ac35e8`.
It was built from the fresh prefix with the existing explicit platform-tag
option, installed in an isolated environment, then exercised outside that venv.
`wheel-inventory.txt` and `wheel-tags.txt` expose package contents and tag;
`local-wheel-binding.json` includes source metadata and exact bytes. The 77 MiB
binary is disposable local output, not checked-in or uploaded release payload.
The default-tag packaging run was not selected as this candidate; the explicit
26.0 tag restricts this artifact to the tested host generation rather than
implying a macOS-11 or formal macOS-15 support claim.

## Failures preserved and closed

- The old preset configure warned of an unused retired option, and its build
  failed on `pycc`; final real preset build succeeds. Intermediate independent
  review found a test helper timeout argument mismatch; the final test fixes it
  and checks all four flags, not just the two component flags.
- Unit inventory initially omitted the already accepted M6 measurement tool:
  one failed, 322 passed, three skipped. `unit-inventory-before.txt` records it;
  the exact inventory assertion was retained and corrected, not weakened.
- Semantic R1: 307 passed, four failed because MLIR_OPT was unset. The SDK
  intentionally has no public `mlir-opt`; an explicit configured LLVM 22.1.8
  tool is required. See `semantic-r1-fail.txt/json`.
- R2 pytest: 311 passed, but PM edited then restored the running script's command
  recording line, causing shell EOF and leaving an earlier fail summary.
  `semantic-r2-shell-fail.txt` records this invalid orchestration run. No product
  semantic defect was established and that run was not accepted.
- R3: script bytes frozen, exact MLIR tool bound, process exit 0 and newly written
  pass summary both independently confirmed. Candidate hashes remain unchanged.
- An initial unit collection under repo Python 3.12 lacked PyYAML from the docs
  tooling environment. The final full unit run used the existing Python 3.14
  pytest environment with PyYAML; no test was removed to bypass collection.

The three unit skips require real Windows filesystem APIs, reparse metadata or
directory handles. Their exact names/reasons are in `unit-skips.json`.

## Reproduce

The commands in `commands.txt` record the actual environment, fresh build,
semantic, unit, package and smoke selections. The current
[runbook](../../../development/release-runbook.md) gives a complete local recipe,
including exact LLVM/MLIR 22.1.8 setup and disposable packaging dependencies.
On another revision, reconfigure/build/install so its metadata matches HEAD and
use new disposable output directories. Installed Python bytes must match that
checkout. Source ownership remains per-source compile → explicit unit link →
same-final CPP/RTL emit, with no whole-design compile/split or retired fallback.

## Acceptance boundary

Independent Sol high review APPROVE binds the manifest above; see
[review](../../../reviews/20261001-m7-local-preview-review.md).
The supported scalar, portless-root, default-clock, empty-static-argument,
serial-simulation profile remains unchanged. SYSTEM/EXPECT B is unapproved.

The formal `v6.1.0` tag points to an older source revision. No release workflow,
tag, public release index/attestation, upload or publication was attempted.
Linux/Windows, formal macOS-15 SDK identity/minimum, wider platform/fault/SDK
matrices, arbitrary Unicode host directories, RSS/throughput, selective backend
rebuilding and parallel simulation remain outside this packet. M7-01 is done;
this is not all M6/M7 or stable release readiness.

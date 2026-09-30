# M7-01: bounded local preview candidate acceptance

Status: done for the bounded local preview. Accepted: 2026-10-01. Started: 2026-10-01. Baseline:
`25152f9c630de6d7d019edcd6c9ed09ce1bebbfc`, initially clean implementation checkout
`codex/gfsim-source-units`. Planning checkout remains separate.

## Objective and authorization

The user's next-step request starts the revision-8 M7 scoped preview exit after
accepted M5/M6-01/M6-02. Validate the already approved C1/C2/C3/R1/M1/C3-SM and
Decision 0283 profile on current macOS arm64. No new source/IR/CLI/schema/ABI,
manifest format, version, platform guarantee, scheduling behavior or cache promise.
This packet produces local acceptance evidence and commits; stable publication,
workflow dispatch, tags and release uploads are outside it.

The supported profile remains portless module-function roots, lexical rules,
explicit registration, finite scalar current/next state, one default clock,
empty static arguments, serial simulation and same-final CPP/RTL emission.
SYSTEM/EXPECT revision B, external typed DUT ports, multiple clocks, memory,
four-state source values and complex parameters remain capability backlog.
M6 platform/fault/SDK coverage, Unicode host-directory hardening, throughput/RSS,
selective backend rebuild and actual parallel simulation remain separate.

## Ownership and candidate construction

| Lane | Instance / actual routing | Exclusive writable files |
| --- | --- | --- |
| Existing-contract build/docs repair | m6_incremental_measurement / Luna high | CMakePresets.json; docs/development/release-runbook.md |
| Independent tests/installed smoke | m5_semantic_oracle_migration / Luna high | tests/system/test_m7_cmake_presets.py; tests/unit/test_m7_retirement_presets.py; relocated compiler and exact tool-inventory tests; independent logs |
| Decomposition/final review | m5_complete_candidate_review / Sol high | Read-only product, gate and evidence review |
| PM integration | root | Retirement gate extension, shared build/install, work packet, profile/status, gate evidence and acceptance |

All agents preserve others' changes and do not recursively delegate. Fresh native
build/install are `.pycircuit_out/m7-01-root` and `.pycircuit_out/m7-01-install`,
from this checkout. No compiler/runtime artifacts are copied from another
worktree. A source correction requires affected evidence and review to be refreshed.

## Initial verified gaps

Root `CMakePresets.json` still selects retired `PYC_BUILD_MLIR_TOOLS`, `pycc` and
`pyc-opt`. Existing retirement scan omits presets and its line patterns cannot
reject structured JSON target references. Repair these remaining approved
hard-break obligations and independently prove old references fail closed.
The release runbook still describes the retired operational route and lacks
current local-preview reproduction. Rewrite its commands and limits against the
current candidate without weakening stable-release/platform protections.

## Acceptance checklist

- [x] Presets use current existing targets/flags and configure/build successfully;
  isolated test output cannot overwrite the default/shared build tree.
- [x] Structured retirement scan rejects restored retired preset target/option;
  real current source/install retirement scan passes.
- [x] Fresh current-checkout native build/install metadata binds this baseline
  plus reviewed dirty overlay; no old cached toolchain is accepted as fresh.
- [x] Current semantic gate covers source compile/link, same-final dual emission,
  source ownership/maps and independent state oracles; native tests pass.
- [x] Installed Runtime-only/CompilerDev and wheel consumers actually build/run;
  package inventory and retired-route rejection remain enforced.
- [x] Current M6 example/no-op/space-path/helper regressions remain green.
- [x] Current documentation, gates, limitations and exact candidate agree;
  unit tests, hooks and strict docs pass or precise gaps are recorded.
- [x] Independent review binds final bytes and evidence; local-preview acceptance
  does not claim the stable release workflow/platform matrix was executed.

## Planned evidence

Use existing current semantic/native/package fixtures rather than a new semantic
engine or rewritten oracle. Gate directory:
`docs/gates/logs/20261001-m7-preview/`. Record all actual commands, status/counts,
selected test/environment identity and byte binding. Preserve stable-release
requirements, including strict decision-status and exact platform identity, even
when they block stable publication. No unavailable check may be reported as pass.

## Acceptance

Independent Sol high review APPROVE binds fourteen-file aggregate
`8b5e7c857bae3abad72e1d2495e9e21f19dbe0d809672756d0f794368b105250`.
[Review](../reviews/20261001-m7-local-preview-review.md) and
[evidence](../gates/logs/20261001-m7-preview/README.md) record the fresh build,
112 real preset steps, native 20/20, semantic 311 passed with two V44 deselections,
unit 324 passed with three Windows-only skips, independent installed/wheel
consumers, current retirement/decision-status gates, hooks and strict docs.
Failed preliminary environment/test/orchestration runs are retained separately.

The wheel is explicitly local macOS 26/arm64, not the formal macOS-15 SDK or a
published release. Existing `v6.1.0` points at older `d4926615...`; no version,
tag, release workflow, release index or publication was changed or dispatched.
M7-01 meets the revision-8 scoped preview exit. Stable release and wider M6/M3
capabilities remain separate; the full migration roadmap is not declared done.

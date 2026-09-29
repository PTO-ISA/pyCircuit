# Migration implementation checkpoint — 2026-09-29

This is a saved development checkpoint, not M2 acceptance or a merge-ready release.
Contracts: approved C2-R1/C2-M1 additions and existing C1/C2/C3 migration contracts.
Planning branch: `codex/gfsim-migration-governance` (bounded milestone revision 8).
Implementation branch: `codex/gfsim-source-units`.
Base: `82f161ea95336fdd15c238e103f13b9f823e8f28`.
Checkout: `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.
`source-hashes.json` binds the staged implementation/test overlay before this commit.

## Fresh bounded verification

Build command: `cmake --build .pycircuit_out/w10-pm/build --target ACIRSimExecutorTests ACIRFinalProgramTests ACIRExecutableBackendClosureTests -j 4`.
Result: exit 0, Ninja reported no work to do. Existing checkout-local LLVM/MLIR 22.1.8 build, macOS arm64.
Each binary was then executed directly from `.pycircuit_out/w10-pm/build/bin/`;
combined stdout/stderr is stored in the corresponding log.

- ACIRSimExecutorTests: exit 0, 9/9 passed.
- ACIRFinalProgramTests: exit 0, 51/51 passed.
- ACIRExecutableBackendClosureTests: exit 1; one failure,
  `ZeroRuleFinalProgramTest.RejectsAnalysisClosureWithRuleButNoSourceCarriers`.
  The expected rejection did not occur. This supersedes the older all-pass
  evidence for this suite and requires investigation before acceptance.
- Staged diff whitespace validation: passed.

## Other open validation

The preceding integration run of the V43 selection had 3 passed / 1 failed:
`test_v43_two_systems_terminate_once_and_reset_reruns`, with
`lifecycle backends disagree`. It was not rerun or resolved in this checkpoint.
No full semantic/release, public CLI/SDK/install or parallel scheduling acceptance
is claimed. Current-candidate M2 core closeout remains outstanding.

Source, tests and unfinished runtime/backend integration are preserved together.
Future work must retain both failing oracles and verify their effect on the
bounded M2 profile; moving delivery features to later milestones does not turn
these failures into passes.

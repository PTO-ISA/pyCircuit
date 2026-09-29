# M2 completion execution

Current instruction (2026-09-29): evaluate M1–M7 and deliver a bounded useful
migration; do not implement every capability or require completeness up front.
This supersedes the earlier all-M2 closure scheduling. Existing semantic
contracts remain authoritative for supported behavior.

Product checkout: `/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit`.
Planning checkout: `/Users/zhoubot/linx-isa/tools/pyCircuit`.
Base: `82f161ea95336fdd15c238e103f13b9f823e8f28` plus preserved dirty overlay.
A working kernel alone does not establish M2 acceptance: bind the declared
core profile to fresh candidate verification and independent review.

## Accepted bounded M2 core — 2026-09-29

M2 is accepted under modernization-plan revision 8 for the single-module and
two-level closed-system fixtures, serial Work/Xfer and their documented finite
numeric profile. This is not full roadmap, public CLI/SDK or M5 acceptance.

Product branch `codex/gfsim-source-units`: base `ee855c1a` plus the content-bound
closeout recorded at `docs/gates/logs/20260929-m2-core-closeout/` in that branch.
Fresh verification: 132 native tests and 27 Python/source/backend tests passed;
no skips, disabled tests or failures. Two V44 selectors remain DEFERRED to M6.
An independent reviewer verified the real Python-per-source → MLIR → common
final IR → executable C++/Verilog chain and the corrected tests.

The old V43 mismatch was a dangling StringRef in RTL test-harness statistics;
owned JSON strings fixed it. The empty clocked-rule rejection was inconsistent
with approved C3; its replacement verifies registered activity at the proper IR
stages, with existing malformed negatives preserved. Both are now verified.

Next: one bounded M4 usable workflow, with M3 additions only as required by that
workflow. Do not reopen all W11/V43–V48 or expand M2 to full capability coverage.

## Historical closure inventory (reassigned below)

- [x] Numeric next-use authority: source assignment, composed numeric witnesses,
  range boundary, RequiredUse, ValueUse and exact yield target/data/enable.
- [x] Python producers for the approved V41/V42 programs, including numeric
  conditions, next writes and ordered checks/observations.
- [x] Graph admission only for independently verified numeric/use packets.
- [x] FinalProgram retains and re-verifies arithmetic, source-use and check
  authority against frozen SSA after source carrier elimination.
- [x] Both emitters execute that same final arithmetic representation.
- [x] Zero-rule source system emits and runs as QUIESCENT at epoch zero.
- [ ] Standard Result/events/ReportStat and successful-commit limit semantics.
- [x] V41/V42 independent five-cycle traces and exact physical register counts.
- [ ] V43 two systems on both backends, terminal Result and reset/rerun.
- [ ] V44 Work/Xfer/parallel scheduling permutations and source-reorder identity.
- [ ] W11 V45 per-source producer/AC/header/TU and parallel CMake graph.
- [ ] W11 V46 one runner/C ABI executor and correct quiescence/activity.
- [ ] W11 V47 clean install/relocated runtime-only consumer execution.
- [ ] W11 V48 invalid final/publication failure preserves old outputs.
- [ ] W12 candidate-bound independent acceptance and M5 deletion manifest.

The future FIFO library, dynamic collections, external typed DUT, multi-clock,
CDC and full four-state capabilities retain their separately recorded contracts
and verification responsibilities. Their unresolved approval status must remain
explicit in the final M2 scope audit; do not silently mark them implemented.

## Current scope override

M2 is now the minimal core defined in modernization-plan revision 8. Complete
V43 runner delivery and required V45/V46 interfaces move to M4. The V47
current-platform clean install/import/run smoke is required when installation
is delivered, before cutover; ordinary V48 invalid-final/output protection is
required when emit/publication is exposed. V44, extended V47 and extended V48
hardening move to M6; full deletion readiness remains M5. These
items are deferred, not passed. The old unchecked list above remains an
inventory of work, not the current M2 stop condition.

M2 closeout requires selecting and rebuilding a frozen current candidate,
checking the two already-supported bounded fixtures (single-module and
two-level-system) and critical negative cases,
and recording the exact supported profile and reproduction command. No new
feature expansion is required for this closeout.

Historical integration state before the accepted closeout: SimExecutor shared tests 9/9; V43 selected
checks 3 passed/1 failed (`lifecycle backends disagree`). This is an open bug in
unfinished integration, not an accepted result. Determine whether it affects
the declared core route; fix any core regression or isolate unfinished delivery
work before claiming M2 accepted. Do not delete or weaken its oracle.

Already-written W11/runtime/reparse work is preserved with its evidence/status;
its existence is not a reason to finish every surrounding feature now. Native
implementation agents are currently interrupted; no broad task is resumed as
part of this planning revision.

## Historical ownership (not active dispatch)

- `m2_numeric_next_core`: dialect/schema/numeric lowering and its build wiring.
- PM: Python numeric-next producer and `PythonImportRules` hook.
- `m2_numeric_next_tests`: independent next-use and source tests/test wiring.
- `m2_zero_rule`: `FinalProgram.cpp` zero-rule admission; frozen for independent
  tests after the implementation report.
- `m2_idle_tests`: independent zero-rule compile/materialize/emit/runtime tests.

Each source freeze requires fresh build, targeted adversarial tests and an
independent review. Shared Graph/Final/backend files will be reassigned only
after the current owner has released them.

## Historical pre-closeout checkpoint — 2026-09-29

Historical status under the former expanded scope: M2 was open, with core
fixtures verified and W11 entering integration. Under the revised scope, core
objectives have evidence, but current-candidate closeout is still required.
The remainder below records implementation history, not automatic dispatch.

Verified before the latest W11 integration:

- Full V41/V42 Python source programs: 11/11 source tests.
- Native composition contracts/adversarial tests: 4/4.
- V41/V42 actual C++/RTL execution: 6/6, including traces, 4/5 physical
  registers, canonical observations, reset/rerun and early-limit rejection.
- Source/math aggregate: 91/91; FinalProgram: 51/51; executable backend: 18/18.
- Independent composition/graph/emitter review: semantic PASS; reported
  formatting defects were repaired.

Implemented, not yet accepted as integrated delivery:

- SimExecutor lifecycle/configuration/statistics/error handling: independent
  direct runtime tests 9/9. Generated runner/C ABI integration remains open.
- Reconstructive FinalProgram loading from serialized final hardware; final
  observations now use canonical ac.logical_types. Fresh-context re-emission
  tests are being added; no acceptance claim yet.
- V43 two-system terminal Result integration and exact source-check diagnostic
  glue. The latest shared build (runtime-build17.log) completed compilation;
  the new combined execution gates have not yet been rerun.

Still open:

- V43 complete terminal lifecycle acceptance and V44 real Work/Xfer/parallel
  permutations plus source reorder mapping.
- W11 standard compile/link/emit path, per-source C++ groups and parallel CMake
  graph, common runner/C ABI delivery, clean Runtime install/relocation and
  publication failure gates (V45–V48).
- W12 frozen-candidate full acceptance and M5 deletion readiness.

Only the PM runs shared Ninja/CMake. Earlier passing logs are historical
candidate evidence; runtime-build17 and later changes require fresh affected
checks before acceptance. Current implementation and test ownership is tracked
in the active native subagent assignments; preserve their files.

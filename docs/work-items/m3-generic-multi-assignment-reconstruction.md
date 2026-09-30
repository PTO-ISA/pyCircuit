# M3 generic multi-assignment reconstruction follow-up

Status: done for this bounded repair; reproduced on existing pre-source-parts helpers during the
M4 TU test design. This was not a new backend regression and was not fixed by the earlier
source-parts package; the repair below now closes it. Preserve the source semantics; do not treat splitting a
rule as an equivalent repair without proving its current/next behavior.

A real Python child with one rule containing `count = incoming` followed by
`outgoing = count` compiles as a source unit, but link refuses final
reconstruction. Native stderr is `final hardware ProposalGraph reconstruction
failed`, then `final hardware program view reconstruction failed`, then
`linked design is not reconstructible by the emit path; this source shape is
not supported yet`. No final output is published.

Minimal source files and observed stderr/exit code are archived under
`docs/gates/logs/20260930-m4-cpp-source-parts/multi-assignment-repro/`.
To reproduce, use tests/system/test_cpp_source_parts.py's source-unit `_compile_source`
helper separately on types.py, counter.py (types header), and test_counters.py
(types/counter headers), then `_link` the units with top
`demo.test_counters.TestCounters`. Helpers must come from the current checkout.

Original repair brief (now completed): trace proposal provenance/use inventory through saved
final reconstruction for both assignments; add a source-level failing-first
regression, repair shared MLIR/link validation within the accepted C1/C2
contract, then prove both C++/RTL behavior. Do not loosen evidence checks, fix
only C++, add a Python semantic fallback, or reopen M2's bounded acceptance.

## Current repair packet

Base: `4fbf7996`; clean product checkout at
`/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit` before the new test.
The user requested the next implementation step. Authority is approved C1/C2
and the newer C2-R1/M1 function/nested-rule contract (planning branch M1 §source
surface, lines 383–396). A captured `nonlocal` reg write proposes next, and its
subsequent read is still old Q. An ordinary local candidate can be reused as
an SSA value. These are distinct oracles and must not be conflated.

- Root-cause investigation: `multi_assignment_debug`, debugger, Sol high,
  read-only ProposalGraph/final reconstruction analysis.
- Independent tests: `multi_assignment_tests`, separate Luna high; exclusive
  tests/system/test_generic_multi_assignment.py, plus disposable evidence.
- Implementation: `unit_pair_fix`, Luna high; exclusive ProposalGraph.cpp.
  No backend/Python/runtime/ODS/schema changes authorized here.
- PM owns integration, shared checkout-local native build and docs/evidence;
  independent Sol code review follows a frozen candidate.

Acceptance: real per-source compile/link, saved-final fresh process emit,
C++/RTL current-Q delay versus local-candidate reuse, grouped C++ where reusable,
reset/rerun, distinct repeated child state/borrowed aliases, and malformed use/
target proof refusals. Require failing-first evidence and preserve all existing
closure/anti-downgrade gates. This is a current capability repair, not a new
primitive or a reopening of M2. M4 declaration headers/generated publication
remain subsequent work.

Parallel M4 readiness investigation is recorded in
[m4-declaration-final-readiness.md](m4-declaration-final-readiness.md); no new
serialized declaration schema is implemented as part of this correctness fix.

## Acceptance — 2026-09-30

The one-file ProposalGraph repair resolves the stale singleton RequiredUse and
yield assumptions. Construction and verification use the same unique target
ordinal and corresponding data/enable pair. Final-use retention and existing
MLIR verifiers are unchanged. Three executable cases prove old-Q reread,
local-candidate reuse and independent enables across both backends and grouped
C++ with reset/rerun. Two proof mutations remain rejected by both backends.

Final new tests 5/5, existing Python/system 68/68 and native 78/78 passed with no
failures/errors/skips/disabled. Independent Sol high review APPROVE, with an
independent 5/5 rerun and both candidate hashes checked. The baseline had two
true source link failures; its two negative cases were setup-blocked, not
executed mutation failures. See [evidence](../gates/logs/20260930-generic-multi-assignment/README.md).

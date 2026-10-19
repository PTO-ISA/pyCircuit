# Compiler benchmark delivery

User objective: fix the compiler findings, integrate the implementation, pass
the affected benchmarks and submit a PR. The audited findings are recorded in
`benchmark/findings/compiler-audit-20261019.md`. Passing prior gates does not
close the remaining findings.

Four-state design follow-up: the user requested a complete frontend/IR/codegen/
System design after discussing reset=X. The proposed
[four-state/System supplement](../rfcs/four-state-system.md) retains strict as
default and places an explicit reset-only if_one policy in common final IR,
shared by native and both RTL adapters. It also specifies literals, exact
verification, finite System stimuli and lossless observations. This replaces the
earlier idea of choosing different reset semantics for different backends.
Implementation and fresh benchmark acceptance remain outstanding; the committed
compiler still has the strict behavior and results recorded below.

## Required work

- [x] Audit each failure through its existing source/IR/backend owner.
- [x] Repair Integer peer normalization and fixed runtime shifts with independent
  native/RTL/four-state and output-protection coverage.
- [x] Repair pure-helper loop arithmetic admission and live ordinal proof.
- [x] Preserve native RTL root identity in public receipts.
- [x] Fix loop resource overreservation without increasing limits or bypassing
  preallocation/audit. Compile and run original vector100r against its upstream TB.
- [x] Support scalar-result generic Modules using existing binding/specialization,
  including parameter overrides and invalid-binding protection.
- [x] Complete public port/domain metadata and external naming at export boundary
  through the existing transparent benchmark adapter; no alias-rewriting compiler
  route was introduced.
- [ ] Implement a reviewed ordinary physical RTL export for sequential benchmark
  modules. Preserve transaction atomicity and explicitly specify simulation versus
  physical sampling obligations; no lifecycle tieoffs or frontend clock wiring.
- [ ] Run all selected VerilogEval/ResBench implementations with unchanged upstream
  testbenches, plus required API/example and affected publication/package gates.
- [ ] Independent current-candidate implementation review; resolve findings.
- [ ] Commit task-owned changes, push, submit upstream PR with exact evidence,
  attach it to the task, and inspect required CI/review state.
- [ ] Complete requested integration under repository branch policy; do not
  equate a submitted PR with a merged change.

## Ownership and sequence

The leader owns shared integration, native build/install, export metadata,
candidate evidence and delivery. Astra source author diagnoses loop accounting;
the independent architect designs physical export; an independent planner traces
generic binding and decomposes remaining interfaces. Implementation assignments
follow their concrete review results. Tests and review remain independent.

Use the current worktree, preserve inherited changes, and reuse existing passes,
runtime leaves, manifests and test entrypoints. No benchmark names, values or
statement recipes may determine product admission. Keep original testbenches,
stimuli, timing and four-state failure oracles. Unsupported unselected dataset
tasks remain explicit; no coverage count is a model-generation or PPA score.

Remote verified at intake: canonical `PTO-ISA/pyCircuit`, default `main` at
`412a180928d3bdf1af3a99f14ed4a21a1c1b172a`; working branch
`codex/typed-module-outputs`, HEAD `dcbd84c0905475ec07089fc9c985316633ae6ddd`.
No matching existing PR at intake. Recheck before publishing.

## Current execution evidence

2026-10-19 continuation: tighter conservative decimal-spelling bounds repaired
the reversal failure. `bounded-for.test` passed with independently executed
shift/concat reversals at widths5/100, workers1/2 and retained rejection/rollback
cases. Original VerilogEval vector100r TB reports0 mismatches/200 samples.
Evidence: `docs/gates/logs/20261019-compiler-delivery/loops-final.log`,
`benchmark/runs/compiler-delivery-vector100r/`.

Before physical export integration, the full selected benchmark baseline is
VerilogEval71 passed/17 managed-interface unsupported and ResBench43 passed/
13 unimplemented. Those are historical candidate results, not final acceptance.

Physical first integration passed11 independent atomicity/four-state tests,
but original dff8 fails its initial-value sample: source initial0 differs from
the autonomous primitive's initialX. This remains open pending generic source
initialization repair; dff and dff8r pass. Generic direct calls pass the base
matrix; root CLI overrides, symbolic concat widths and genuine parameter
forwarding remain under active implementation/diagnosis. No PR yet.

Later candidate: full scalar generic suite15/15 passed, including root overrides,
dependent defaults, imported symbolic forwarding and strict malformed-binding
protection. Physical startup now uses verified closed typed initializer constants;
VerilogEval86/88 passes. m2014_q4c/lfsr32 fault on their first edge because the
unchanged upstream TB has not initialized reset; strict versus native-Verilog
reset-X policy is pending direct user clarification. The compiler does not hide
that fault. Remaining acceptance covers wide/nested/Collection startup and
negative physical projection cases; refreshed API/example gates are running.

Final local checkpoint validation: API23/23, Examples13/13, unit627 passed/
5 skipped/82 deselected, strict docs build and changed-file pre-commit checks
passed. Physical runtime/startup24 cases, additional5 initialization rejection/
synthesis cases, isolated projection4 cases and scalar generic15 cases passed;
reviewers closed the reported implementation and test-isolation findings.
Benchmark VerilogEval remains86/88 executed implementations (68 unselected),
ResBench43/43 implemented (13 unimplemented), and four output-corruption
negative controls are detected by original oracles. Full nightly/PPA remain
NOT RUN. No upstream PR/merge claim is made while the reset-X decision is open.

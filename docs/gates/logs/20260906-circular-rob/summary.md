# Circular ROB and multi-state rule evidence

Date: 2026-09-06

## Scope

- Decisions 0171-0177
- inferred typed system boundaries;
- scalar and fixed-list `ac.var` storage selection;
- shared-state lexical arbitration and exact footprint verification;
- zero-input/zero-output guarded rules;
- heterogeneous multi-owner prepare/publish/no-fail commit;
- generated four-entry circular ROB.

## Commands and results

```text
python -m unittest discover -s tests/python/agentic-circuit/python_frontend -p test_queue_frontend.py
71 tests: PASS

cmake --build .pycircuit_out/acir/dev-llvm22 --target check-acir -j4
151 tests: PASS

.pycircuit_out/acir/dev-llvm22/bin/GfsimTests --gtest_brief=1
255 tests: PASS

.pycircuit_out/acir/dev-llvm22/bin/CodeGenTests --gtest_brief=1
98 tests: PASS

.pycircuit_out/acir/dev-llvm22/bin/ACIRModelAnalysisTests --gtest_brief=1
19 tests: PASS

python -m unittest tests.integration.agentic-circuit.e2e.test_queue_codegen
16 tests: PASS, 1 skipped because the external DavinciOO reference trace fixture is absent

python tools/agentic-circuit/check-contracts.py
PASS: 12 public schemas, 36 stdlib components, epoch 0.5, LLVM 22.1.8

python flows/tools/check_decision_status.py ... --require-existing-evidence
PASS: 177 rows, 0 deferred

mkdocs build --strict
PASS

git diff --check
PASS
```

## ROB acceptance evidence

`examples/agentic-circuit/state/circular_rob.py` contains no explicit
source/sink, Queue/Table/Reg, readiness, pop/push, reservation, publication, or
commit operations. The generated-model integration proves:

- four-entry full/empty distinction and retained fifth allocation;
- allocation and retirement output backpressure without partial state updates;
- out-of-order completion and in-order retirement;
- fixed-width head/tail wrap across more than four allocations;
- per-slot generation stale-completion rejection;
- flush/recovery epoch rejection and post-flush forward progress;
- atomic updates across scalar cursor/occupancy state, indexed entries, and
  Queue effects.

## Remaining flow work

- QueueGraph still flattens module definitions/instances; specialization-keyed
  generated module reuse remains open.
- Generated integration harnesses still scan dispatch rows each tick; inferred
  activation adjacency and hot-work scheduling remain open.
- General multi-arm CFG alternatives, conditional proposals, and multiple
  selected outputs remain open beyond the accepted ROB subset.

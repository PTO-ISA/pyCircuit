# Agentic Circuit first stateful rule evidence

Date: 2026-09-05

Decision: 0163

Scope: one Table, one Queue input/output, one optional committed Entry read,
one complete Entry replace, staged ACIR lowering, QueueGraph, and grouped gfsim
execution. PYC/RTL remain an explicit unsupported provisional Table boundary.

## Results

| Lane | Result |
| --- | --- |
| Agentic Python excluding environment-specific build/tool harnesses | PASS: 226 tests, 2 skipped |
| Stateful rule JIT native MLIR/C++ smoke | PASS |
| Stateful rule end-to-end Python → Frozen ACIR → QueueGraph → generated gfsim C++ | PASS: two same-index writes return zero then the first committed value; final Table contains the second value |
| ACIR/ACSim lit suite | PASS: 141 tests |
| ACIR dialect C++ tests | PASS: 1845 tests |
| QueueGraph code-generation C++ tests | PASS: 96 tests |
| gfsim C++ tests | PASS: 250 tests |
| Public IR inventory and generated coverage ledger | PASS |
| Decision status | PASS: 163 rows, 0 deferred |
| Agentic repository contracts | PASS: epoch 0.5, LLVM 22.1.8 |
| API hygiene | PASS |
| MkDocs strict | PASS |

## Proved contracts

- Python exposes only a committed Table read, ordinary Table assignment, and
  payload return; it contains no explicit Queue effects, checks, handshake,
  reservation, commit, rollback, or atomic-region syntax.
- `ac.table.propose` is verifier-owned, firing-local, complete-replace-only,
  type exact, Table-owned, and statically bounds-safe in this slice.
- Frozen QueueGraph planning re-verifies the module and repeats type,
  same-Table, constant/dynamic bounds, and exclusive-writer checks before C++
  emission; direct Plan mutation regressions cover these fail-closed paths.
- Rule passes emit Table-aware effects, handshake, exclusive scheduling, and
  marker-free stateful `ac.firing`; pure rules continue to canonicalize to
  `ac.transform`.
- QueueGraph preserves proposal identities and lowers to
  `gfsim::QueueTableTransition`; generated code compiles and runs.
- Generated-model output backpressure preserves the first Table value and the
  unconsumed second input until the selected output becomes available.
- PYC rejects the graph with `unsupported provisional Table` rather than
  silently approximating state semantics.

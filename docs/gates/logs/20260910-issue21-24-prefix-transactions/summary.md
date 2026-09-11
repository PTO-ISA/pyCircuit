# Issues 21 and 24 ordered-prefix transaction evidence

## Scope

This run covers Decision 0239 Table multi-selection and Decision 0240 ordered
multi-lane Queues.

Decision 0239 now has one typed `ac.table.choose` with `2*N` scalar results,
static Python tuple use, signed and unsigned ordering, deterministic tie-breaks,
round-robin accepted-only cursor state, once-per-Epoch evaluation, and one
atomic valid-prefix consumer. Direct Table reads are grouped before planning;
partial, mixed, direct-write, or otherwise ungrouped consumers fail closed.
Table PYC/RTL parity remains blocked on Decision 0241, so Decision 0239 is
`implemented-unverified`.

Decision 0240 preserves one typed Queue identity with explicit lanes, rate, and
canonical ordinals. Runtime batch prepare/publish implements whole-prefix
backpressure, simultaneous dequeue/append, reset, and deterministic FIFO
ordering. Canonical PYC admits generic direct Queues and one-to-one pure
transform chains at latency one; other multi-lane topologies fail closed. The
same admitted fixtures execute in generated gfsim C++, PYC C++, and Verilator.

## Results

- LLVM22 affected-target build: PASS.
- ACIR lit: PASS, 202 of 202 tests.
- Native ACIR/analysis/QueueGraph/gfsim suites: PASS, 5 of 5 suites.
- Focused gfsim prefix tests: PASS, 7 of 7 tests.
- Focused QueueGraph/codegen tests: PASS, 4 of 4 tests.
- Agentic Circuit Python frontend: PASS, 275 passed and 1 skipped.
- PYC C++ execution for direct and transform lane fixtures: PASS, 2 of 2.
- Verilator behavior for direct and transform lane fixtures: PASS through lit.
- Address/undefined sanitizer runs for generated direct and transform gfsim
  fixtures: PASS; leak detection is not supported on this macOS platform.
- PYC inventory, repository contracts, decision-status validation, strict
  MkDocs, and `git diff --check`: PASS.

Exact commands, stdout, stderr, and return codes are stored beside this file.

## Boundary

Decision 0241 remains the authority for Table-to-PYC register-bank lowering.
Decision 0239 must not be promoted to implemented-verified until that later
Table C++/Verilog parity evidence exists. Decision 0240 does not admit
multi-lane fork, merge, route, barrier, or heterogeneous topology in PYC; those
extensions fail at the explicit admission boundary rather than scalarizing the
Queue identity.

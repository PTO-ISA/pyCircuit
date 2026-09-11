# Multi-owner QueueGraph module reuse gate summary

Decision 0181 carries heterogeneous state-owner commit groups into reusable
module specializations. One generated `StatePair` class contains the policy,
two Table members, and one `QueueStateTransition`; each placement constructs an
independent instance of that complete state layout.

## Evidence

- `tests/mlir/agentic-circuit/Transforms/queue-multi-owner-module-freeze.mlir`
  - frozen plan preserves ordered cursor and total state writes;
  - generated C++ uses `StateTransitionPlan` and `QueueStateTransition`;
  - the specialization class contains two Table members and compiles as C++20.
- `QueueGraphPlanTest.ReusesMultiOwnerModuleWithAtomicIndependentInstanceState`
  - generated source contains one class and two instance members;
  - left inputs `3,5` report combined cursor/total values `4,10`;
  - right input `10` independently reports `11`.
- `cmake --build .pycircuit_out/acir/dev-llvm22 --target check-acir -j 8`
  - 155/155 lit tests passed.
- `.pycircuit_out/acir/dev-llvm22/bin/CodeGenTests`
  - 102/102 tests passed.

## Remaining scope

The reusable path still needs combined multi-rule/multi-owner modules, internal
Queue graphs, nested instances, Python `@ac.module` lowering, and incremental
activation.

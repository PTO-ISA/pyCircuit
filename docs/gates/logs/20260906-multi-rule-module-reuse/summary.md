# Multi-rule QueueGraph module reuse gate summary

Decision 0180 extends a reusable stateful specialization to multiple Queue
ports and multiple firing rules over one owner. The specialization body and C++
class remain singular while each placement owns independent state and runtime
IDs.

## Evidence

- `tests/mlir/agentic-circuit/Transforms/queue-multi-rule-module-freeze.mlir`
  - two-input/two-output module freezes and plans one specialization body;
  - two firings retain priorities 0 and 1;
  - generated C++ contains both typed transitions and compiles as C++20.
- `QueueGraphPlanTest.ReusesMultiRuleModuleAndPreservesOwnerLocalLexicalArbitration`
  - generated source contains one class and two module instances;
  - simultaneous left inputs produce `1` then `3`, proving losing-input
    retention and recomputation from committed state;
  - the independent right instance produces `10` through its second rule.
- `cmake --build .pycircuit_out/acir/dev-llvm22 --target check-acir -j 8`
  - 154/154 lit tests passed.
- `.pycircuit_out/acir/dev-llvm22/bin/CodeGenTests`
  - 101/101 tests passed.

## Remaining scope

Reusable modules still need multi-owner transactions, internal Queue graphs,
nested instances, Python `@ac.module` lowering, and incremental activation.

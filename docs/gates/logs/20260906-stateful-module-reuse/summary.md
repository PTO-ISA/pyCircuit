# Stateful QueueGraph module reuse gate summary

Decision 0179 separates specialization code reuse from per-instance committed
state. One frozen `Accumulator` definition produces one specialization plan and
one generated implementation class, while two placements construct independent
Table and firing runtime objects.

## Evidence

- `tests/mlir/agentic-circuit/Transforms/queue-stateful-module-freeze.mlir`
  - module-local Table references freeze and reverify;
  - both instances share one specialization fingerprint;
  - canonical plan contains one firing/Table body;
  - generated C++ compiles as C++20.
- `QueueGraphPlanTest.ReusesStatefulModuleImplementationWithIndependentPersistentState`
  - source contains one implementation class and two members of that class;
  - left inputs `1,2` produce accumulated results `1,3`;
  - right input `10` independently produces `10`;
  - each placement receives independent firing and Table IDs.
- `cmake --build .pycircuit_out/acir/dev-llvm22 --target check-acir -j 8`
  - 153/153 lit tests passed.
- `.pycircuit_out/acir/dev-llvm22/bin/CodeGenTests`
  - 100/100 tests passed.

## Remaining scope

Module reuse still needs multi-rule/multi-owner state, arbitrary Queue arity,
nested instances, Python `@ac.module` lowering, and incremental activation.

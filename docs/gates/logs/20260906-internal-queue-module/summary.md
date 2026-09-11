# Internal Queue ownership in nested modules

Decision 0184 preserves an internal Queue inside each repeated parent module
instance. One local transform feeds one reusable child without promoting the
intermediate Queue to the root model.

## Evidence

- `tests/mlir/agentic-circuit/Transforms/queue-mixed-nested-module.mlir`
  - plan contains local transform -> `prepared` Queue -> child binding;
  - generated parent class owns the internal `SimQueue` and compiles as C++20.
- `QueueGraphPlanTest.OwnsInternalQueuesPerMixedNestedSpecializationInstance`
  - generated source contains one parent class and one child class;
  - two parent instances independently execute `5 -> 7` and `10 -> 12`.
- `cmake --build .pycircuit_out/acir/dev-llvm22 --target check-acir -j 8`
  - 158/158 lit tests passed.
- `.pycircuit_out/acir/dev-llvm22/bin/CodeGenTests`
  - 105/105 tests passed.

## Remaining scope

Mixed modules still need arbitrary internal Queue graphs, multiple local blocks,
local state plus child instances, Python `@ac.module` lowering, and incremental
activation.

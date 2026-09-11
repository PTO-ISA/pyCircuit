# Nested QueueGraph module reuse gate summary

Decision 0183 preserves specialization reuse across one nested module level.
Planning builds child specializations before parents; codegen emits one child
class, one wrapper class, and recursively partitions dense runtime IDs for each
wrapper placement.

## Evidence

- `tests/mlir/agentic-circuit/Transforms/queue-nested-module-freeze.mlir`
  - canonical plan contains Wrapper -> Increment specialization nesting;
  - generated source contains one class per specialization and compiles as
    C++20.
- `QueueGraphPlanTest.PreservesNestedSpecializationReuseWithoutFlatteningChildBodies`
  - root has two Wrapper instances but source contains one Wrapper class and one
    Increment class;
  - inputs `5,10` execute through independent nested objects and produce
    `6,11`.
- `cmake --build .pycircuit_out/acir/dev-llvm22 --target check-acir -j 8`
  - 157/157 lit tests passed.
- `.pycircuit_out/acir/dev-llvm22/bin/CodeGenTests`
  - 104/104 tests passed.

## Remaining scope

Nested modules still need mixed local blocks, internal Queue storage, Python
`@ac.module` lowering, and incremental activation.

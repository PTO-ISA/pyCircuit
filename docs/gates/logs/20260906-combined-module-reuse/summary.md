# Combined multi-rule/multi-owner module gate summary

Decision 0182 combines multiple firing rules and multiple persistent owners in
one reusable QueueGraph specialization. Each instance constructs the owner
union independently, while each firing binds only its own Queue and owner
subset through `QueueStateTransition`.

## Evidence

- `tests/mlir/agentic-circuit/Transforms/queue-multi-rule-multi-owner-module.mlir`
  - one specialization contains two rules and two Tables;
  - both rules preserve ordered multi-owner footprints and lexical priority;
  - generated C++ contains two typed `QueueStateTransition` members and
    compiles as C++20.
- `QueueGraphPlanTest.ReusesCombinedMultiRuleMultiOwnerModuleWithAtomicArbitration`
  - generated source contains one class and two module instances;
  - simultaneous left requests report `2` then `5`, proving losing-input
    retention and recomputation across both owners;
  - the independent right instance reports `11`.
- `cmake --build .pycircuit_out/acir/dev-llvm22 --target check-acir -j 8`
  - 156/156 lit tests passed.
- `.pycircuit_out/acir/dev-llvm22/bin/CodeGenTests`
  - 103/103 tests passed.

## Remaining scope

The next implementation step is to consolidate duplicated stateful codegen,
then support internal Queue graphs, nested instances, Python `@ac.module`
lowering, and incremental activation.

The first cleanup pass centralizes field-merge policy generation across
single-owner, multi-owner, multi-rule, and combined specializations. All five
module-reuse runtime regressions and their MLIR compilation tests remain green;
the second pass normalizes owner/port bindings and centralizes transition
policy, constructor, member, and dispatch emission. The three obsolete
shape-specific stateful branches were deleted. `QueueGraphGenerator.cpp`
dropped from 3428 to 2937 lines while preserving all module behaviors.

# QueueGraph module-freeze gate summary

Decision 0178 establishes the first hierarchy-preserving QueueGraph contract.
The compiler now accepts module-local Queue scopes, derives stable definition
and specialization fingerprints during topology freeze, preserves the selected
system and elaborated instance owners, verifies the complete seal on a second
pass, and extracts one reusable specialization body plus instance bindings.
The first gfsim lowering emits that specialization class once and constructs
two independently bound instances.

## Evidence

- `cmake --build .pycircuit_out/acir/dev-llvm22 --target check-acir -j 8`
  - 152/152 Agentic Circuit lit tests passed.
- `tests/mlir/agentic-circuit/Transforms/queue-module-freeze.mlir`
  - two instances of `@Increment` receive one identical specialization;
  - the root and reusable definition receive compiler-derived fingerprints;
  - freezing the already frozen result is byte-identical;
  - canonical QueueGraph JSON contains one module body and two references.
- `QueueGraphPlanTest.PreservesReusableModuleSpecializationAndRunsIndependentInstances`
  - generated C++ contains one specialization class and two instance members;
  - both instance paths transform and retire their own value successfully;
  - dispatch IDs preserve broadcast, left instance, right instance, then sink
    lexical order.
- `.pycircuit_out/acir/dev-llvm22/bin/CodeGenTests`
  - 99/99 tests passed.
- `git diff --check`
  - passed after the implementation and documentation update.

## Remaining scope

Reusable gfsim generation currently covers a pure one-input/one-output leaf.
The next slice must extend the same specialization-keyed structure to stateful
multi-rule modules, nested instances, arbitrary Queue arity, and independent
persistent state.

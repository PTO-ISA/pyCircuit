# Primary-checkout C2-F02 verification

Candidate: 29b424bf. Working directory: /Users/zhoubot/linx-isa/tools/pyCircuit.
All seven files match the independent Sol high review-b manifest in ../candidate.json. Worktree 34abbb6c was cherry-picked without product/test content changes.

```sh
cmake --build .pycircuit_out/acir/dev-llvm22 --target ACIRDialect acir-opt-internal ACIRSourceContractsTests ACIRTypesTests -j 6
.pycircuit_out/acir/dev-llvm22/bin/ACIRSourceContractsTests --gtest_brief=1
.pycircuit_out/acir/dev-llvm22/bin/ACIRTypesTests --gtest_brief=1
.pycircuit_out/ac-venv/bin/lit -sv --filter='PyCircuitMLIR :: ACIR/(c2-source-contracts(-invalid)?|types-(valid|invalid))\.mlir$' .pycircuit_out/acir/dev-llvm22/tests/mlir
```

All exit codes 0. Current-checkout LLVM/MLIR22.1.8 build passed. Combined F01/F02 GTest 28/28, existing type GTest 6/6, lit selected 4 of 273 and passed 4. Total selected 38. No native binaries were copied between checkouts. Text logs have terminal trailing whitespace removed.

Scope is closed C2 LogicalType/StaticType/StaticValue/Default records and private matching under an injected record resolver. Callback symbol consistency, finite nominal graphs, ordered fields, range/kind/list/default checks are covered; actual header authority/import/link/operations/passes/backends remain unimplemented by this package.

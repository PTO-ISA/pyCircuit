# Primary-checkout integration verification

Candidate: d104dae0 on codex/gfsim-migration-governance.
Working directory: /Users/zhoubot/linx-isa/tools/pyCircuit.
All ten implementation/test files match the independently reviewed worktree manifest in ../candidate.json. Worktree commit dc74190c was cherry-picked without content changes.

Commands and results:

```sh
cmake --build .pycircuit_out/acir/dev-llvm22 --target ACIRDialect acir-opt-internal ACIRSourceContractsTests ACIRTypesTests -j 6
.pycircuit_out/acir/dev-llvm22/bin/ACIRSourceContractsTests --gtest_brief=1
.pycircuit_out/acir/dev-llvm22/bin/ACIRTypesTests --gtest_brief=1
.pycircuit_out/ac-venv/bin/lit -sv --filter='PyCircuitMLIR :: ACIR/(c2-source-contracts(-invalid)?|types-(valid|invalid))\.mlir$' .pycircuit_out/acir/dev-llvm22/tests/mlir
```

Every command exited 0. Fresh primary-checkout source recompilation passed with LLVM/MLIR22.1.8. GTests: 11 new plus 6 existing. Lit discovered 273, selected 4 and passed 4; 269 were excluded. Duplicate-library linker warnings occurred but no build failure. No native artifacts were copied between checkouts.

Scope: approved C2-F01 MathInt/type and private SourceSpan/PathComponent/Site validators. No importer, operation/pass, link, final-stage closure, backend or full migration result is claimed.

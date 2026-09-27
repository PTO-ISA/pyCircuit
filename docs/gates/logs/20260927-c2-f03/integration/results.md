# Primary-checkout C2-F03 verification

Candidate: 4b84de00. Working directory: /Users/zhoubot/linx-isa/tools/pyCircuit.
All six files match the independent Sol high review-b manifest. Worktree commit 0c38929d was cherry-picked without content changes.

```sh
cmake --build .pycircuit_out/acir/dev-llvm22 --target ACIRDialect acir-opt-internal ACIRSourceContractsTests ACIRTypesTests -j 6
.pycircuit_out/acir/dev-llvm22/bin/ACIRSourceContractsTests --gtest_brief=1
.pycircuit_out/acir/dev-llvm22/bin/ACIRTypesTests --gtest_brief=1
.pycircuit_out/ac-venv/bin/lit -sv --filter='PyCircuitMLIR :: ACIR/(c2-source-contracts(-invalid)?|types-(valid|invalid))\.mlir$' .pycircuit_out/acir/dev-llvm22/tests/mlir
```

All commands exited 0. Current-source LLVM/MLIR22.1.8 build passed, contract GTest 36/36, existing types 6/6, selected lit 4/4 of 273 discovered. Total selected 46. No native artifacts copied between checkouts; terminal trailing whitespace is removed from saved logs.

Independent review caught host-dependent native path semantics. Explicit POSIX absolute and Windows root-name checks now reject C:/model.py and C:model.py on all hosts while retaining ordinary relative pkg:variant/model.py. This is structural validation, not filesystem/root/symlink/case-fold proof.

Scope remains the private identity/provenance dictionaries and u32/u64 domains. Real AST facts, declaration authority, SSA ownership, source/header/link and backend behavior are future integration responsibilities.

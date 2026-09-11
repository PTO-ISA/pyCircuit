# Issue 22 bounded Table PYC/RTL evidence

Decision 0241 admits a bounded Table profile through canonical PYC as one
explicit `pyc.reg` per flattened Entry. Independent admission checks cover rank
4, 256 entries, 256-bit Entries, 65,536 total bits, four writers, and
overflow-safe rejection beyond each boundary. Generated PYC contains no
`sync_mem` and no residual ACIR Table operation.

The admitted path covers typed and multidimensional initialization, flattened
index checks, old-state get/read, Queue- and state-driven rules, outputless
transactions, field and masked writes, replace-after-field commit order,
count-greater-than-one selection, shared match/choice reuse, signed/unsigned min/max, accepted-only
round-robin cursor advance, valid-prefix backpressure, and deterministic writer
priority. The original direct/native gfsim suite and the new PYC C++/Verilator
cycle tests agree on old-state publication, replace ordering, stalls, cursor
advance, and arbitration.

LLVM/MLIR 22, pycc, ACIR, gfsim, and `libpyc6_runtime` were rebuilt from the
current checkout. ASAN passes all 23 Table runtime tests. The macOS ASAN build
cannot instrument the Homebrew LLVM/MLIR libraries consistently and reports a
library-internal poisoned allocator access during MLIR context construction;
the same Table generator fixtures and runtime tests pass under UBSAN with
`halt_on_error=1`.

See `commands.txt`, `summary.json`, and the adjacent bounded logs for exact
evidence.

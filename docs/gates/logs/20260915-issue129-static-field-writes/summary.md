# Issue 129 static Table field writes

Decisions: 0154, 0236, 0241, 0263.

Implemented static, fail-closed field proposal recognition for explicit Tables.
The frontend requires a `with_fields` chain rooted in the same owner and an
AST-equivalent index, canonicalizes fields in Entry declaration order, merges
consecutive basic-block updates, and preserves guarded branch footprints.
Module-local explicit Tables retain the same intent through storage selection.
Dynamic disjoint assertions and runtime conflict obligations remain deferred to
the later Issue 129 slice.

## Results

- Focused field-assignment frontend: 20 passed.
- Full Agentic Python frontend: 403 passed, 2 skipped.
- Writer-arbitration lit: 1 passed.
- Module-local storage-selection lit: 1 passed.
- Freeze, QueueGraph plan, C++ generation, and PYC generation: passed in the
  focused native Table backend test.
- Table backend: 8 passed.
- Table PYC parity: 3 passed.
- Contracts: 48 passed, 2 failed only because the pre-existing untracked
  repository-root `designs/` directory violates release-layout policy.
- `check-acir`: 235 passed, 2 unsupported, 9 failed. The same nine failures
  existed at baseline and are unrelated source/build skew in hierarchy, memory,
  helper-symbol, and PYC source-provenance lanes; both changed lit tests pass.
- The integrated `run_agentic_circuit.sh` lane stopped during CMake generation:
  the local preset names Homebrew LLVM paths and its LLVM export requires a
  missing `zstd::libzstd_static` target. No G0/G1/G2 test executed in that lane;
  the focused current-checkout G1/G2 tests above provide the semantic evidence.

The native merge fixture initializes one row with a nonzero untouched field,
runs two outputless rules that write disjoint fields at the same dynamic index,
and proves both fields commit while the untouched field and all other rows are
preserved. QueueGraph JSON retains the two exact field footprints and generated
C++ uses two `FieldMerge` writers.

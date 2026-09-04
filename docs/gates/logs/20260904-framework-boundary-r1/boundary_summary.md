# Consumer/framework boundary closure

Run ID: `20260904-framework-boundary-r1`

This run validates Decision 0158: pyCircuit owns the reusable language,
compiler, runtime, simulator, backend, examples, and verification contracts;
complete product designs and their integration tooling are consumer-owned and
out of tree.

## Boundary changes

- Removed the in-tree Linx, Janus, and XiangShan integration roots, the Zybo
  platform tree, and the LinxCore frontend example.
- Removed consumer ISA/QEMU/FPGA/performance tests and scripts from root CI and
  framework gates.
- Removed `pyc_linxtrace.hpp`, `pyc_konata.hpp`, and the Linx decoder JIT
  complexity allowlist.
- Kept generic trace, probe, testbench, Python/CMake package, MLIR, gfsim, C++,
  and Verilog contracts.
- Updated repository management so consumer repositories pin pyCircuit and own
  compatibility evidence.

Git history remains the migration source for the removed files. No files were
written into external consumer repositories because those local checkouts had
unrelated uncommitted work.

## Verification

- Release-layout boundary: passed; `integrations/`, `platforms/`, and the old
  LinxCore frontend example root are forbidden.
- Root unit tests: 13/13 passed.
- Python frontend: 128 passed, 2 optional skipped.
- CLI: 53/53 passed.
- ACIR/ACSim lit: 138/138 passed.
- CTest: 15/15 passed.
- AC G2 and public rule-retirement PYC C++/Verilog lane: passed.
- Fresh integrated install contains no Linx, Janus, XiangShan, Zybo, or Konata
  files.
- pyCircuit examples, normal simulations, nightly simulations, and V6 semantic
  regressions: passed.
- API hygiene, repository contracts, generated IR coverage ledger, strict
  158-row decision status, and `mkdocs build --strict`: passed.

# Canonical Agentic PYC-to-Verilog path

## Verify the integrated Verilog bridge {#VER-PYC-VERILOG-001}
<!-- ndf: kind=verif level=must layer=L3 status=stable verifies=D-BLOCK-MODEL-001 -->

Semantic Agentic designs use the shared PYC compiler and qualified RTL
selection path:

```text
verified ACIR
  -> acir-queue-pycgen
  -> canonical textual PYC IR
  -> pycc + pyc-select-rtl-primitives
  -> Verilog + closed RTL source inventory and selection record
```

`ac.popcount(value)` lowers to vendor-neutral `pyc.popcount`. Only pycc's
Verilog selection pass may introduce `pyc.rtl.comb` and the qualified
`pyc_popcount_primitive` implementation. `acc -emit-verilog` routes verified
ACIR through canonical PYC and the sibling `pycc`; it does not select or
hard-code semantic primitives itself.

Leading and trailing zero-count helpers follow the same route through one
`pyc.count_zeros` operation with a static `direction` parameter. Their
all-zero result is `N`, and only the Verilog selection pass introduces the
catalog-owned `pyc_count_zeros_primitive` module.

Example:

```bash
acir-queue-pycgen model.verified.ac.mlir > model.pyc
PYC_PRIMITIVES_DIR="$PWD/library/verilog" \
  pycc model.pyc --emit=verilog --out-dir build/verilog \
  --hierarchy-policy=strict --inline-policy=off
```

For a constrained host, build only the required producers and keep Ninja
single-threaded:

```bash
cmake --build .pycircuit_out/toolchain/build \
  --target acir-queue-pycgen pycc -j1
```

`tests/mlir/agentic-circuit/CodeGen/popcount.mlir` proves ACIR-to-semantic-PYC.
`tests/mlir/agentic-circuit/CodeGen/count-leading-zeros.mlir` proves the same
boundary for leading-zero count.
`tests/system/test_primitive_selection.py` then runs the canonical PYC through
pycc, checks the selection inventory and BSD source/license ownership, and lints the closed
output with Verilator. `pyc-primitives-smoke.sv` retains bounded FIFO/arbiter
runtime coverage and now uses the same canonical popcount module.

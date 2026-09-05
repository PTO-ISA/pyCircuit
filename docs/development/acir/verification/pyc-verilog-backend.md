# Canonical Agentic PYC-to-Verilog path

## Verify the integrated Verilog bridge {#VER-PYC-VERILOG-001}
<!-- ndf: kind=verif level=must layer=L3 status=stable verifies=D-BLOCK-MODEL-001 -->

Semantic Agentic designs use the shared PYC compiler and qualified RTL
selection path:

```text
frozen ACIR
  -> acir-queue-pycgen
  -> canonical textual PYC IR
  -> pycc + pyc-select-rtl-primitives
  -> Verilog + digest-verified RTL source closure and selection manifest
```

`ac.popcount(value)` lowers to vendor-neutral `pyc.popcount`. Only pycc's
Verilog selection pass may introduce `pyc.rtl.comb` and the qualified
`pyc_popcount_primitive` implementation. The legacy
`acir-queue-veriloggen.py` textual compatibility emitter does not select or
hard-code semantic primitives; unsupported semantic PYC operations fail and
must be routed through pycc.

Leading and trailing zero-count helpers follow the same route through one
`pyc.count_zeros` operation with a static `direction` parameter. Their
all-zero result is `N`, and only the Verilog selection pass introduces the
digest-verified `pyc_count_zeros_primitive` module.

Example:

```bash
acir-queue-pycgen model.frozen.ac.mlir > model.pyc
PYC_PRIMITIVES_DIR="$PWD/library/verilog" \
  pycc model.pyc --emit=verilog --out-dir build/verilog \
  --hierarchy-policy=strict --inline-policy=off
```

For a constrained host, build only the required producers and keep Ninja
single-threaded:

```bash
cmake --build build/dev-llvm22 --target acir-queue-pycgen pycc -j1
```

`tests/mlir/agentic-circuit/CodeGen/popcount.mlir` proves ACIR-to-semantic-PYC.
`tests/mlir/agentic-circuit/CodeGen/count-leading-zeros.mlir` proves the same
boundary for leading-zero count.
`tests/system/test_primitive_selection.py` then runs the canonical PYC through
pycc, checks the selection manifest and BSD source digest, and lints the closed
output with Verilator. `pyc-primitives-smoke.sv` retains bounded FIFO/arbiter
runtime coverage and now uses the same canonical popcount module.

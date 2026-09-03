# In-tree PYC-to-Verilog bridge

## Verify the integrated Verilog bridge {#VER-PYC-VERILOG-001}
<!-- ndf: kind=verif level=must layer=L3 status=stable verifies=D-BLOCK-MODEL-001 -->

The `acir-queue-veriloggen.py` entrypoint closes the first integrated backend
slice without introducing a second ACIR lowering:

```text
frozen ACIR
  -> acir-queue-pycgen
  -> canonical textual PYC IR
  -> PYC compatibility emitter
  -> self-contained Verilog (PYC runtime modules embedded)
```

The runtime modules under `library/verilog/` are the small
cycle-level primitives migrated from pyCircuit's runtime surface: `pyc_fifo`,
`pyc_reg`, `pyc_popcount`, and `pyc_rr_arbiter`.  The bridge currently covers
the operations needed by the golden Popcount and round-robin Queue slices:
FIFO, register, wiring, constants, mux/boolean/integer expressions, concat,
extract, Popcount, and round-robin Arbiter. Unsupported PYC operations fail
explicitly instead of silently producing incomplete RTL.

Pure Python compute regions spell population count as `ac.popcount(value)`.
The bridge also preserves canonical `pyc.assert` operations as simulation-only
Verilog checks guarded by synthesis translation directives; failure semantics
are not silently removed from Verilator runs.

Example:

```bash
python compiler/acir/tools/acir-queue-veriloggen.py tests/mlir/agentic-circuit/CodeGen/arbiter-verilog.mlir \
  --pycgen build/dev-llvm22/bin/acir-queue-pycgen \
  --emit-pyc /tmp/arbiter.pyc.mlir \
  --output /tmp/arbiter.v
```

For a constrained WSL host, build only the producer needed by this bridge and
keep Ninja single-threaded:

```bash
cmake --build build/dev-llvm22 --target acir-queue-pycgen -j1
python compiler/acir/tools/acir-queue-veriloggen.py tests/mlir/agentic-circuit/CodeGen/arbiter-verilog.mlir \
  --pycgen build/dev-llvm22/bin/acir-queue-pycgen --timeout 120 \
  --output /tmp/arbiter.v
```

The timeout guards against an accidentally started full build or malformed
input keeping the lowering process alive.  It does not start a compiler build;
the bridge consumes an already-built `acir-queue-pycgen` executable.

The generated Verilog is self-contained, so Verilator/Yosys can consume one
file. This is an integration bridge, not yet the generic registry-driven
`pycc` replacement. The next step is to replace the textual compatibility
emitter with the shared PYC MLIR dialect/emitter once the repositories agree on
one LLVM/MLIR toolchain.

The same path is covered by `tests/mlir/agentic-circuit/CodeGen/popcount-verilog.mlir`; the
fixture exercises packed-field extraction (`i12` to `i8`) before the qualified
`pyc.popcount` primitive.  `tests/mlir/agentic-circuit/CodeGen/pyc-primitives-smoke.sv` provides a
bounded N=2/N=3/N=4 arbitration, popcount, and FIFO simulation. The smoke is
part of `PycVerilogBackendTests` whenever Verilator is available.

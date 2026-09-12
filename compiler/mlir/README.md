# `compiler/mlir`: MLIR dialect + tools

This directory contains the MLIR-based implementation of the `pyc` dialect and
its command-line tools:

- `pyc-opt`: `mlir-opt`-style tool with `pyc` dialect + passes
- `pycc`: compile `.pyc` (MLIR) to Verilog or C++ via template libraries

## Build

Build from the repository root through the top-level `CMakeLists.txt`; see the
[installation guide](../../docs/getting-started/installation.md).

You can also build this subproject standalone if you already have an LLVM+MLIR build/install.

This example assumes an existing LLVM/MLIR 22 install or build tree.

```bash
cmake -G Ninja -S compiler/mlir -B .pycircuit_out/mlir-standalone \
  -DMLIR_DIR=/path/to/llvm-22/lib/cmake/mlir \
  -DLLVM_DIR=/path/to/llvm-22/lib/cmake/llvm

ninja -C .pycircuit_out/mlir-standalone pyc-opt pycc
```

## Passes

### `pyc-eliminate-wires`

Eliminates trivial `pyc.wire` + `pyc.assign` pairs when safe (single driver that
dominates all reads), and removes dead wires. This reduces netlist noise and
helps subsequent CSE/constprop.

`pycc` runs this pass by default before emission.

### `pyc-comb-canonicalize`

Combinational simplifications, currently focused on mux canonicalization:

- collapses nested muxes with the same select
- rewrites some `i1` mux patterns into simpler boolean logic

`pycc` runs this pass by default before emission.

### `pyc-fuse-comb`

Fuses consecutive pure combinational ops (`pyc.add/mux/and/or/xor/not/constant`) into
`pyc.comb` regions. This is a codegen-oriented transform intended to enable:

- flattened Verilog emission (`assign` instead of many tiny module instantiations)
- inlined C++ combinational evaluation (fewer tiny objects / calls)

`pycc` runs this pass by default before emission.

### `pyc-check-flat-types`

Verifies that the IR is fully lowered to flat hardware-carrying types
(integers + `!pyc.clock`/`!pyc.reset`) before emission. This is a safety net
similar in spirit to FIRRTL's type-lowering: pyCircuit's Python frontend packs
bundles/vectors into integers, so aggregate types should never reach the PYC IR.

`pycc` runs this check by default.

### `pyc-prune-ports`

Module-level cleanup pass that prunes unused `func.func` arguments and updates
`func.call` sites. This changes the externally visible interface, so it is
**not** run by default in `pycc`, but can be useful for internal
refactors or design-space exploration flows.

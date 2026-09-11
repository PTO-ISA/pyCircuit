# Pipeline

pyCircuit uses a two-stage compile pipeline:

1. Frontend (Python): source scan + JIT elaboration + `.pyc` emission
2. Backend (`pycc`): MLIR passes + emit C++ and/or Verilog

## Frontend

Frontend responsibilities:

- strict API contract scan (entry file + local imports)
- JIT elaboration of `@module` / `@function` / `@const`
- materialize `@module(value_params=...)` as runtime boundary input ports
- emit one `.pyc` per specialized module
- emit a deterministic `project_manifest.json`
- emit a testbench `.pyc` payload from `@testbench`

All emitted modules are stamped with:

- `pyc.frontend.contract = "pycircuit"`

## Backend (`pycc`)

Backend responsibilities:

- verify required frontend contract attrs (`pyc-check-frontend-contract`)
- verify value-param metadata arity/alignment (`pyc.value_params` + `pyc.value_param_types`)
- inline helper functions and run cleanup/verification passes
- preserve `@module` hierarchy boundaries in strict mode (default: `--hierarchy-policy=strict`)
- emit:
  - C++ model (`--emit=cpp`)
  - Verilog netlist (`--emit=verilog`)
  - testbench text (for `.pyc` files containing `pyc.tb.payload`)

Default backend hierarchy policy:

- `--hierarchy-policy=strict`
- `--inline-policy=off` for hierarchy-preserving module builds
- strict mode fails compilation if frontend module symbol set changes after lowering passes

## CLI entrypoints

Emit a single `.pyc`:

```bash
python3 -m pycircuit.cli emit <design.py> -o out.pyc
```

Build a project (multi-module + testbench):

```bash
python3 -m pycircuit.cli build <tb_or_top.py> --out-dir <dir> --target cpp|verilator|both --jobs <N>
```

Simulation (Verilator):

```bash
python3 -m pycircuit.cli build <tb.py> --out-dir <dir> --target verilator --run-verilator
```

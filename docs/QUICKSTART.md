# Quickstart

Run these commands from the repository root.

## Build the compiler

```bash
bash flows/scripts/pyc build
export PYC_TOOLCHAIN_ROOT="$PWD/.pycircuit_out/toolchain/install"
```

## Verify the frontend

```bash
PYTHONPATH=compiler/frontend python3 -c \
  "from pycircuit import CycleAwareSignal, compile_cycle_aware; print('pyCircuit 6 frontend ready')"
```

## Build the counter project

```bash
PYTHONPATH=compiler/frontend \
python3 -m pycircuit.cli build \
  designs/examples/counter/tb_counter.py \
  --out-dir /tmp/pyc_counter \
  --target both \
  --jobs 8
```

The build produces frontend manifests, C++ sources and executable artifacts,
Verilog, and Verilator inputs under `/tmp/pyc_counter`.

## Run smoke gates

```bash
bash flows/scripts/run_examples.sh
bash flows/scripts/run_sims.sh
```

The examples lane checks compilation and semantic contracts. The simulation lane
checks C++ and Verilator behavior.

## Continue learning

- [V6 tutorial](v6_PyCircuit_Tutorial.md)
- [V6 language specification](v6_PyCircuit_Specification.md)
- [Testing and gates](development/testing-and-gates.md)

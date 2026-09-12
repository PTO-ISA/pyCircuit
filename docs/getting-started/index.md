# Getting Started

Use this section to install pyCircuit, choose the right frontend, and run a
design through the current pyCircuit 6 toolchain.

## Choose your path

| Goal | Start here |
| --- | --- |
| Build synthesizable hardware from signals and logical cycles | [pyCircuit quickstart](quickstart.md#build-the-counter-example) |
| Model queues, resources, processes, and architecture state | [Choose Agentic Circuit](choose-a-frontend.md#use-agentic-circuit-for-architecture-models) |
| Set up a compiler-development checkout | [Installation guide](installation.md#full-source-toolchain) |
| Learn the language step by step | [pyCircuit 6 tutorial](tutorial.md) |

## Requirements

- Linux or macOS
- Python 3.10 or later for `pycircuit`
- Python 3.11 or later for the integrated Agentic Circuit toolchain
- CMake and Ninja for native builds
- LLVM/MLIR 22.1.8 for compiler development
- Verilator for Verilog simulation lanes

## Fastest frontend-only setup

This path emits PYC MLIR without building the native compiler:

```bash
git clone https://github.com/PTO-ISA/pyCircuit.git
cd pyCircuit

python3 -m venv .venv
source .venv/bin/activate
python -m pip install -e "python/semantic-core"
python -m pip install -e .

mkdir -p .pycircuit_out/quickstart
python -m pycircuit.cli emit \
  examples/pycircuit/basics/counter/counter.py \
  -o .pycircuit_out/quickstart/counter.pyc
```

Build the native toolchain when you need C++ simulation, Verilog, ACIR/ACSim,
or gfsim:

```bash
bash flows/scripts/pyc build
export PYC_TOOLCHAIN_ROOT="$PWD/.pycircuit_out/toolchain/install"
```

## Documentation map

- [Installation](installation.md)
- [Choose a frontend](choose-a-frontend.md)
- [Quickstart](quickstart.md)
- [Tutorial](tutorial.md)
- [Language and API reference](../reference/index.md)
- [Troubleshooting and diagnostics](../reference/diagnostics.md)

Generated files belong under `.pycircuit_out/`. Keep complete product designs
and product-specific verification in their owning consumer repositories.

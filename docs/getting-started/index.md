# Getting Started

Use this section to choose a frontend, install the integrated repository, and
run either the pyCircuit 6 hardware flow or the Agentic Circuit architecture
flow.

## Prerequisites

- Python 3.10 or later for `pycircuit`
- Python 3.11 or later for `agentic_circuit` and the integrated AC gates
- LLVM/MLIR 22 (for compiler backend)
- CMake 3.20+
- Ninja build system

## What is covered

- [Installation](installation.md)
- [Choose a frontend](choose-a-frontend.md)
- [Repository quickstart](../QUICKSTART.md)
- [V6 tutorial](../v6_PyCircuit_Tutorial.md)
- [V6 language specification](../v6_PyCircuit_Specification.md)

## Installation options

### Full development setup

```bash
# Install system dependencies (Ubuntu)
sudo apt-get install cmake ninja-build python3 python3-pip clang wget
LLVM_INSTALL_SCRIPT_SHA256=03878e08f47b66cc95bc4b544b0db3c6d9ce8d60e6cf2492ae357984330a9eae
wget --https-only https://apt.llvm.org/llvm.sh
printf '%s  %s\n' "$LLVM_INSTALL_SCRIPT_SHA256" llvm.sh | sha256sum --check --strict
chmod +x llvm.sh
sudo ./llvm.sh 22
sudo apt-get install llvm-22-dev mlir-22-tools libmlir-22-dev

# Clone and build
git clone https://github.com/PTO-ISA/pyCircuit.git
cd pyCircuit

# Build the compiler
bash flows/scripts/pyc build
```

### Python frontend only

```bash
# Install Python package
python3 -m pip install -e .

# Use the frontend to emit MLIR
PYTHONPATH=compiler/frontend \
python -m pycircuit.cli emit your_design.py -o your_design.pyc
```

### Release packages

The reserved distribution name is `pycircuit-hisi` to avoid the unrelated
`pycircuit` project that already exists on PyPI. The import path remains
`pycircuit`. Do not use a PyPI installation command until a matching PTO-ISA
release has been published; use a release wheel from the repository's release
artifacts or build from source.

## Next Steps

After installation:

- follow the [V6 tutorial](../v6_PyCircuit_Tutorial.md) for Cycle-Aware Signal
  hardware and testbenches; or
- read the [ACIR overview](../acir/index.md) for architecture, process, queue,
  and resource modeling.

# Getting Started

Use this section to install pyCircuit 6 and run the supported cycle-aware
design flow.

## Prerequisites

- Python 3.10 or later
- LLVM/MLIR 22 (for compiler backend)
- CMake 3.20+
- Ninja build system

## What is covered

- [Installation](installation.md)
- [Repository quickstart](../QUICKSTART.md)
- [V6 tutorial](../v6_PyCircuit_Tutorial.md)
- [V6 language specification](../v6_PyCircuit_Specification.md)

## Installation Options

### Full development setup

```bash
# Install system dependencies (Ubuntu)
sudo apt-get install cmake ninja-build python3 python3-pip clang wget
wget https://apt.llvm.org/llvm.sh
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
PYTHONPATH=compiler/frontend python -m pycircuit.cli emit your_design.py
```

### Published package

```bash
python3 -m pip install pycircuit-hisi
```

The distribution name is `pycircuit-hisi` to avoid the unrelated `pycircuit`
project that already exists on PyPI. The import path remains `pycircuit`.

## Next Steps

After installation, follow the [V6 tutorial](../v6_PyCircuit_Tutorial.md) for a
CycleAwareSignal design and testbench.

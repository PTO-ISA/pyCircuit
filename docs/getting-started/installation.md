# Installation

Choose the smallest installation profile that matches the work you need to do.

## Requirements

| Component | Frontend only | Full toolchain |
| --- | --- | --- |
| Operating system | Linux or macOS | Linux or macOS |
| Python | 3.10+ | 3.11+ recommended |
| CMake and Ninja | Not required | Required |
| LLVM/MLIR 22.1.8 | Not required | Required |
| Verilator | Not required | Required for Verilog simulation |

On macOS, install the native dependencies with Homebrew:

```bash
brew install cmake ninja python@3 llvm@22 verilator
export PATH="$(brew --prefix llvm@22)/bin:$PATH"
```

On Ubuntu or Debian, install CMake, Ninja, Python, a C++ compiler, and the LLVM
22 development packages from the
[official LLVM package repository](https://apt.llvm.org/). Verify the selected
toolchain before configuring the build:

```bash
LLVM_CONFIG="$(command -v llvm-config-22 || command -v llvm-config)"
MLIR_OPT="$(command -v mlir-opt-22 || command -v mlir-opt)"
"$LLVM_CONFIG" --version
"$MLIR_OPT" --version
python3 --version
```

## Frontend-only editable install

Use this profile to author Python and emit PYC MLIR:

```bash
git clone https://github.com/PTO-ISA/pyCircuit.git
cd pyCircuit

python3 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install -e "python/semantic-core"
python -m pip install -e .
```

Verify the installation:

```bash
python -c "import pycircuit; print(pycircuit.__name__)"
python -m pycircuit.cli --help
```

The distribution name is `pycircuit-hisi`; the Python import is `pycircuit`.
An editable frontend install does not place `pycc` on `PATH`.

## Full source toolchain

Install development and documentation dependencies, then run the canonical
build wrapper:

```bash
python -m pip install -e "python/semantic-core"
python -m pip install -e ".[dev,docs]"
bash flows/scripts/pyc build

export PYC_TOOLCHAIN_ROOT="$PWD/.pycircuit_out/toolchain/install"
export PATH="$PYC_TOOLCHAIN_ROOT/bin:$PATH"
pycc --version
```

The build wrapper detects LLVM, configures Ninja, builds PYC plus the integrated
ACIR/ACSim/gfsim components, and stages the install tree under
`.pycircuit_out/toolchain/install/`.

For manual configuration or constrained build hosts, see the
[development guide](../development/index.md) and
[testing matrix](../development/testing-and-gates.md).

## Add Agentic Circuit

Agentic Circuit is a separate distribution and import namespace in the same
repository:

```bash
python -m pip install -e "python/agentic-circuit[test]"
agentic-circuit --help
```

The Python package provides authoring and CLI surfaces. Native compilation uses
the ACIR/ACSim/gfsim tools built by the full source toolchain. Run the integrated
gate once to validate the complete local environment:

```bash
PYC_GATE_RUN_ID=local-ac-$(date +%Y%m%d-%H%M%S) \
bash flows/scripts/run_agentic_circuit.sh
```

## Install a release wheel

Download the wheel for your platform from
[GitHub Releases](https://github.com/PTO-ISA/pyCircuit/releases/latest), then
install the local file:

```bash
python3 -m pip install /path/to/pycircuit_hisi-6.0.0-*.whl
pycc --version
python3 -m pycircuit.cli --help
```

Platform wheels include the compiler and runtime assets. The semantic-core and
Agentic Circuit distributions remain separate universal wheels. Use exactly the
asset set published by one release.

## Verify the setup

```bash
pytest tests/unit -m unit
bash flows/scripts/run_examples.sh
```

System and Verilog simulation checks require the full toolchain and Verilator:

```bash
pytest tests/system -m system
bash flows/scripts/run_sims.sh
bash flows/scripts/run_semantic_regressions_v6.sh
```

## Troubleshooting

### LLVM is not found

Ensure `llvm-config` reports version 22.1.8 and export the package directories:

```bash
LLVM_CONFIG="$(command -v llvm-config-22 || command -v llvm-config)"
export LLVM_DIR="$("$LLVM_CONFIG" --cmakedir)"
export MLIR_DIR="$(dirname "$LLVM_DIR")/mlir"
bash flows/scripts/pyc build
```

### `pycc` is not found

Build the full toolchain and add its staged binary directory to `PATH`:

```bash
export PYC_TOOLCHAIN_ROOT="$PWD/.pycircuit_out/toolchain/install"
export PATH="$PYC_TOOLCHAIN_ROOT/bin:$PATH"
```

### A build needs a clean reconfiguration

Keep the existing checkout and ask CMake to rebuild the configured tree:

```bash
cmake --build .pycircuit_out/toolchain/build --clean-first --parallel
```

If configuration itself is stale, create a new ignored build directory instead
of deleting source or evidence files.

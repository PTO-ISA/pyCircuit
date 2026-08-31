# Installation Guide

This guide covers setting up the pyCircuit development environment.

## System Requirements

| Component | Minimum Version | Recommended Version |
|-----------|---------------|---------------------|
| Python | 3.10 | 3.14 |
| LLVM | 22 | 22.1.8 |
| CMake | 3.20 | 3.28+ |
| Ninja | 1.10 | Latest |

## Install System Dependencies

### Ubuntu/Debian

```bash
# Update package lists
sudo apt-get update

# Install build tools
sudo apt-get install -y cmake ninja-build python3 python3-pip clang wget

# Install LLVM/MLIR 22 (Ubuntu 22.04+)
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 22
sudo apt-get install -y llvm-22-dev mlir-22-tools libmlir-22-dev

# Verify installation
llvm-config-22 --version
mlir-opt --version
```

### macOS

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install build tools
brew install cmake ninja python@3

# Install LLVM 22 with MLIR
brew install llvm@22
# Add LLVM to PATH
echo 'export PATH="$(brew --prefix llvm@22)/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc

# Verify installation
llvm-config --version
```

## Clone and Build

```bash
# Clone the repository
git clone https://github.com/PTO-ISA/pyCircuit.git
cd pyCircuit

# Configure with CMake
LLVM_CONFIG="${LLVM_CONFIG:-$(command -v llvm-config-22 || command -v llvm-config)}"
: "${LLVM_CONFIG:?LLVM 22 llvm-config not found}"
LLVM_DIR="$("$LLVM_CONFIG" --cmakedir)"
MLIR_DIR="$(dirname "$LLVM_DIR")/mlir"

cmake -G Ninja -S . -B .pycircuit_out/toolchain/build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PWD/.pycircuit_out/toolchain/install" \
  -DLLVM_DIR="$LLVM_DIR" \
  -DMLIR_DIR="$MLIR_DIR"

# Build and stage the compiler toolchain
ninja -C .pycircuit_out/toolchain/build pycc pyc-opt pyc6_runtime
cmake --install .pycircuit_out/toolchain/build --prefix "$PWD/.pycircuit_out/toolchain/install"

# Verify the build
./.pycircuit_out/toolchain/install/bin/pycc --version
```

## Alternative: Use Build Script

```bash
# The project includes a build script that handles LLVM detection
bash flows/scripts/pyc build
```

## Alternative: Install a Release Wheel

```bash
python3 -m pip install /path/to/pycircuit_hisi-<version>-py3-none-<platform>.whl

# The wheel ships the matching toolchain inside site-packages.
pycc --version
python3 -m pycircuit.cli --help
```

The wheel is platform-specific because it embeds `pycc`, the runtime archive,
and the required LLVM/MLIR shared libraries. Use the wheel that matches your
OS and architecture. A single wheel now covers Python 3.10+ on that platform.

Published package install command:

```bash
python3 -m pip install pycircuit-hisi
```

The distribution name is `pycircuit-hisi` to avoid the existing unrelated
`pycircuit` package on PyPI. The import path remains `pycircuit`, and the CLI
entrypoints remain `pycircuit`, `pycc`, and `pyc-opt`.

## Install Python Package

```bash
# Install the frontend package in development mode
python3 -m pip install -e .

# Verify installation metadata
python3 -c "from importlib.metadata import version; print(version('pycircuit-hisi'))"
```

Editable install is frontend-only. It does not provide `pycc` on `PATH`; build
the toolchain with `bash flows/scripts/pyc build` and export
`PYC_TOOLCHAIN_ROOT="$PWD/.pycircuit_out/toolchain/install"`, or install a
release wheel instead.

## Verify Your Setup

```bash
# Run the smoke test
bash flows/scripts/run_examples.sh

# Should output something like:
# Compiling counter... OK
# Compiling calculator... OK
# Compiling fifo_loopback... OK
```

## Troubleshooting

### LLVM Not Found

If CMake can't find LLVM, set the paths explicitly:

```bash
export LLVM_DIR=/path/to/llvm/lib/cmake/llvm
export MLIR_DIR=/path/to/mlir/lib/cmake/mlir
cmake -G Ninja -S . -B .pycircuit_out/toolchain/build ...
```

### Python Version Issues

pyCircuit requires Python 3.10+. Check your version:

```bash
python3 --version
```

If you need to install a newer Python version:

```bash
# Ubuntu
sudo apt-get install python3.11 python3.11-venv

# macOS
brew install python@3.11
```

### Build Errors

Clean and rebuild:

```bash
rm -rf .pycircuit_out/toolchain
cmake -G Ninja -S . -B .pycircuit_out/toolchain/build ...
ninja -C .pycircuit_out/toolchain/build clean
ninja -C .pycircuit_out/toolchain/build pycc
```

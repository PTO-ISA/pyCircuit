# pyCircuit 6.1

**Write hardware in Python. Simulate it in C++ or Verilog.**

pyCircuit compiles typed Python modules, state variables and rules through MLIR
into one verified hardware representation. The same system produces a GFSIM
simulation executable and Verilog that runs with Verilator.

There is one frontend: `pycircuit`. Names such as `ac` and `pyc` are ordinary
Python import aliases, not different languages or compilation modes.

## Hello counter

A complete source system needs only Python:

```python
from pycircuit import bits, log, rule, system


@rule
def increment(count):
    log("info", "count", count)
    count = count + 1


@system
def HelloCounter():
    count: bits[8] = 0
    increment(count)
```

The compiler infers storage, clock/reset connections and write enables. It also
generates the simulation driver, RTL testbench, build files and runtime limits.
No handwritten C++ or SystemVerilog is needed for this example.

With an installed toolchain:

```sh
pycircuit run examples/hello_counter --cycles 5
pycircuit run examples/hello_counter --target verilog --cycles 5
```

Use `--toolchain /path/to/install` when the compiler is not bundled with the
Python package. Each cycle has low and high sampling epochs; the counter logs
`0, 0, 1, 1, 2, 2, 3, 3, 4, 4`. Build outputs remain under
`.pycircuit_out/run/hello_counter/`, including independently buildable C++ and
Verilog directories and their `pycircuit_sim` binaries.

See the [quickstart](docs/getting-started/quickstart.md),
[system execution design](docs/architecture/system-execution.md), and
[examples](examples/README.md).

## Build from source

The compiler requires Python 3.11+, CMake 3.25+, Ninja, a C++20 compiler and
LLVM/MLIR **22.1.8**. Verilog simulation additionally needs Verilator. Generated
C++ models can use the Runtime package without LLVM.

```sh
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -e .
bash flows/scripts/pyc build
export PYC_TOOLCHAIN_ROOT="$PWD/.pycircuit_out/toolchain/install"
export PATH="$PYC_TOOLCHAIN_ROOT/bin:$PATH"
pycircuit run examples/hello_counter --cycles 5
```

The [installation guide](docs/getting-started/installation.md) covers compiler
and Runtime-only builds.

## How it works

```text
Python modules and systems
        ↓ syntax capture
MLIR type, dependency and effect analysis
        ↓ lowering and verification
Source-owned units → explicit link closure → final hardware IR
                                               ↙           ↘
                                  GFSIM C++ executable   System Verilog
```

`pycircuit compile`, `pycircuit link` and `pycircuit emit` remain available for explicit build graphs.
`pycircuit run` composes those same operations and builds the selected generated
simulation. Each source is compiled independently; C++ and Verilog consume the
same final IR.

Work reads the old state. Whole-system checks precede Xfer. A failed epoch
commits neither state nor clock history and publishes no source observations.
Independent verification models belong in framework tests, outside the DUT and
compiler.

## Documentation and development

- [Python language](docs/reference/language.md)
- [Modules, systems and execution](docs/reference/language-specification.md)
- [Known limitations and follow-up work](docs/development/known-limitations.md)
- [Contributing](CONTRIBUTING.md)
- [Testing and nightly coverage](docs/development/testing-and-gates.md)

Unsupported constructs fail explicitly. Retired frontends and compiler aliases
are not fallback paths. Long coverage, reference-model and platform matrices
run through the existing nightly and release workflows.

pyCircuit is licensed under [BSD-3-Clause](LICENSE).

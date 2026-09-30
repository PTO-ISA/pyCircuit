# pyCircuit

pyCircuit captures a supported subset of ordinary Python hardware descriptions,
checks their meaning in MLIR, and emits C++ or Verilog from one verified final
design. The active product route is `pycircuit compile` → `pycircuit link` →
`pycircuit emit`.

The current M5 profile is intentionally bounded: one portless function
`@module`, nested `@rule` definitions and explicit registrations, one default
clock, no external input/output ports, and no static arguments. It supports
closed designs with scalar state, explicit current/next updates, and nested
modules within that profile. It does not provide queues, a full `@system`
runner contract, memory/CDC, multiple clocks, four-state source values, or an
external typed DUT interface. Unsupported constructs fail with diagnostics.

## Install and build

Install Python 3.11+, CMake, Ninja, a C++20 compiler, and LLVM/MLIR 22.1.8 for
the compiler developer profile. The runtime-only CMake component has no
LLVM/MLIR dependency. From a checkout:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -e .
cmake -S . -B .pycircuit_out/build -G Ninja \
  -DPYC_BUILD_COMPILER_DEV=ON \
  -DPYC_BUILD_TESTING=OFF \
  -DPYC_BUILD_RUNTIME_LIB=ON
cmake --build .pycircuit_out/build
cmake --install .pycircuit_out/build --prefix .pycircuit_out/install
export PATH="$PWD/.pycircuit_out/install/bin:$PATH"
```

See [installation](docs/getting-started/installation.md) for the two CMake
profiles and [quickstart](docs/getting-started/quickstart.md) for a complete
small design.

## Compile, link, and emit

Each Python source is compiled into its own published source unit. Link the
complete unit closure to a final `design_top.ac`, then emit either backend:

```bash
mkdir -p .pycircuit_out/units
pycircuit compile -c src/top.py --source-root src \
  --package-prefix demo -o .pycircuit_out/units/top
pycircuit link .pycircuit_out/units/top --top demo.top.Top \
  -o .pycircuit_out/design_top.ac
pycircuit emit .pycircuit_out/design_top.ac --target cpp \
  -o .pycircuit_out/cpp
pycircuit emit .pycircuit_out/design_top.ac --target verilog \
  -o .pycircuit_out/verilog
```

For imported modules, compile every source independently and pass each
published interface unit to its parent with `-I`; list all implementation and
declaration units when linking. Generated CMake builds a `pycircuit_system`
runner and `libpycircuit_dut` against the installed Runtime. Set a finite limit,
for example `{"deadlock_window":null,"max_domain_cycles":{},"max_ticks":100,"schema":"agentic-model-config","version":"1"}`, in the runner
configuration and invoke `pycircuit_system --config config.json`. Runtime
execution is silent by default; pass `--events <path>` or `--events -` when
event output is wanted.

## CMake consumers

A Runtime-only consumer needs just the installed runtime package:

```cmake
find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)
target_link_libraries(my_model PRIVATE pycircuit::pyc6_runtime)
```

The Runtime component does not find LLVM. `CompilerDev` exports compiler
development targets and requires exact LLVM/MLIR 22.1.8. See the [language
reference](docs/reference/language.md), [frontend guide](docs/development/agent-frontend-guide.md),
and [testing and gates](docs/development/testing-and-gates.md).

## Hard break

The prior CycleAwareSignal/JIT, structural builder, Agentic Circuit/QueueGraph,
PYC C++ compiler and their command aliases are retired from the active product
route. There is no fallback mode. The [M5 migration guide](docs/development/m5-migration.md)
records the bounded replacement workflow and the capabilities that remain
backlog. M5 is accepted for that profile; see the [candidate-bound review](docs/reviews/20261001-m5-cutover-review.md) and its verification evidence.

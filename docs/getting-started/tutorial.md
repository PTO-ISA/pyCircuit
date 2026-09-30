# Build and run the source-owned counter

This tutorial uses the active portless/default-clock profile and the checked-in
source-owned example at `examples/pycircuit/counter/`. Its source files are
compiled independently, its imports resolve through published interfaces, and
its linked final artifact feeds both C++ and Verilog emitters.

## Install the toolchain

Install Python 3.11+, CMake, Ninja, a C++20 compiler, and LLVM/MLIR 22.1.8.
Follow the source install steps in [installation](installation.md); the
CompilerDev install prefix must be available to CMake.

For example, if the prefix is `.pycircuit_out/install`, set:

```bash
export PYCIRCUIT_INSTALL="$PWD/.pycircuit_out/install"
```

## Build the C++ runner

```bash
cmake -S examples/pycircuit/counter \
  -B .pycircuit_out/counter -G Ninja \
  -DCMAKE_PREFIX_PATH="$PYCIRCUIT_INSTALL"
cmake --build .pycircuit_out/counter
cmake -S .pycircuit_out/counter/cpp \
  -B .pycircuit_out/counter/run -G Ninja \
  -DCMAKE_PREFIX_PATH="$PYCIRCUIT_INSTALL"
cmake --build .pycircuit_out/counter/run
```

The first build runs one `pycircuit compile` producer per Python source,
explicitly links `design_top.ac`, and emits source-owned C++ groups. The second
build compiles those groups against the installed Runtime and links
`pycircuit_system` and `libpycircuit_dut`.

Run the model with the example's finite three-cycle configuration:

```bash
.pycircuit_out/counter/run/pycircuit_system \
  --config examples/pycircuit/counter/config.json --events -
```

Omit `--events` to run silently. The runner only emits events when a sink is
explicitly selected.

## Emit Verilog

The generated project also has a Verilog target using the same final design.
Select the Verilog target when configuring the example project and use a
Verilator installation for downstream simulation. Verilog emission is not evidence for
external ports, full `@system`, or other unsupported profile features.

## Scope

`Counter` demonstrates explicit rule registration, current-state reads,
`nonlocal` next-state proposals, module connections, and source-owned output.
The module root is portless and static arguments are empty. This tutorial does
not use a testbench DSL or promise full `@system`/EXPECT behavior.

See the [language reference](../reference/language.md) and
[M5 migration guide](../development/m5-migration.md) for the supported boundary
and future capability backlog.

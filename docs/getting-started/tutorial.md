# Build and run the hello counter

The source in `examples/hello_counter/` is a complete `@system`: persistent
state, a rule and a log statement. pyCircuit generates its simulation closure.

## Run either backend

Follow [installation](installation.md), then run:

```sh
pycircuit run examples/hello_counter --cycles 5
pycircuit run examples/hello_counter --target verilog --cycles 5
```

Use `--toolchain /absolute/installed/prefix` if needed. Use `--workers 2` for
native parallel execution. Verilog execution uses Verilator. The native path
does not require Verilator or Icarus.

Each clock cycle has two sampling epochs. Both engines log the old-state
sequence `0, 0, 1, 1, 2, 2, 3, 3, 4, 4` and terminate at epoch 10. Source checks
run before any state commit, and observations publish only after success.

## Inspect the generated closure

The default build directory is `.pycircuit_out/run/hello_counter/`:

- `units/hello_counter/`: source-owned implementation and published interface.
- `hello_counter.ac`: verified linked hardware IR.
- `cpp/`: generated modules, typed DUT, main and CMake project.
- `verilog/`: generated modules, simulation top and CMake project.
- `simulation/<target>/bin/pycircuit_sim`: compiled simulation executable.

The generated directories build independently against the installed Runtime:

```sh
cmake -S .pycircuit_out/run/hello_counter/cpp -B .pycircuit_out/hello-cpp \
  -G Ninja -DCMAKE_PREFIX_PATH="$PYC_TOOLCHAIN_ROOT"
cmake --build .pycircuit_out/hello-cpp --target pycircuit_sim
.pycircuit_out/hello-cpp/bin/pycircuit_sim --cycles 5
```

Select the generated `verilog` directory instead to build the Verilator binary;
its cycle argument is `+cycles=5`. The public `pycircuit run` command handles
these backend command differences.

See [system execution](../architecture/system-execution.md) for the shared
semantics and [the language reference](../reference/language.md) for admission.

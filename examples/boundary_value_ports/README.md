# Combinational lane boundaries

The design has one 32-bit input `seed` and one 32-bit output `acc`.
Three lanes produce `(seed + 6) mod 2^32`, `(seed + 12) mod 2^32`, and `seed`;
their sum is reduced to 32 bits. The third lane retains its disabled selector,
gain 7 and bias 11. The original `seed=10` case produces `acc=48`.

`Lane` and `Sum3` are reusable modules with direct same-source calls, preserving
each modular boundary. `first = Lane(seed, 1, 5, 1)` creates one lane instance
and returns its typed value; the other two calls create separate instances.
`Sum3(first.y, second.y, third.y)` consumes their results. No state or clock
delay is introduced. The two enabled lanes and disabled lane can execute
independently before the summing instance.

The source uses `ac.u32` inputs and modular bits arithmetic. `Lane` and `Sum3`
return `LaneResult(y: ac.u32)`. The root returns
`BoundaryValuePortsResult(acc: ac.u32)` in one physical `result` packet.
The disabled lane uses a pure conditional expression, retaining four-state
selection semantics without a storage enable.

```sh
export CMAKE_BUILD_TYPE=Release
cmake -S examples/boundary_value_ports -B /absolute/build/boundary-value-ports -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/boundary-value-ports --parallel 4
ctest --test-dir /absolute/build/boundary-value-ports --output-on-failure --no-tests=error
```

The design reuses existing bits arithmetic, selection and module composition.
Independent native and RTL oracles cover the
original result, intermediate and total wrap, high bits, consecutive samples,
and four-state arithmetic followed by known-input recovery. See the
[generated-output guide](GENERATED.md) for actual public-flow IR, C++ and RTL.

## Scenario bench

`bench.py` exports `example_boundary_value_ports.bench.ExerciseBoundaryValuePorts`. Its immutable
Scenario Table contains 16 original input/expected pairs plus a separate all-zero tail row. Each row pairs the
original inputs with the independent expected values; constructing this data
adds no persistent state or clock cycle.

The full-width index selects `phase` below 16 and row 16 otherwise. The zero expected tail preserves the original disabled-check fallback; it is not the DUT result for zero input, which remains 18.

The original DUT calls, u64 phase initialization and wrapping advance, guards,
assertions, messages, logs and check-before-advance order remain unchanged.
Independent source proof verifies all active values, the complete known-u64
fallback and the remaining source structure. Arbitrary X/Z phase equivalence
is not claimed; the closed phase retains its known initialization and update.

Both source versions pass the existing module and system CTests. Thirty complete
first/warm/public system traces preserve all 32 observations and a successful
terminal result at 32 sampling epochs, including workers 1/2 and RTL. Only
changed AST positions are normalized in comparisons. This system covers the
original 16-cycle known-input scenario; physical oracle coverage remains separate.

The module checks 16 known samples and four native unknown/recovery cases. The separate original Icarus macro repeats its four unknown/recovery cases; those tasks do not print FOUR markers.

Use fresh build directories and export `CMAKE_BUILD_TYPE=Release` before
configuring and running tests. The public runner creates nested CMake builds,
which do not inherit the outer cache. Cycle counts, configuration and the
180-second command budgets remain unchanged. Complete source-import and
transformed artifacts match published source units; public and measured final
IR and generated outputs are byte-identical.

```sh
export CMAKE_BUILD_TYPE=Release
pycircuit run examples/boundary_value_ports --target cpp --cycles 16
pycircuit run examples/boundary_value_ports --target verilog --cycles 16
```

The Icarus macro is a separate check using the original testbench and generated
module RTL. After the standalone build above, run:

```sh
build=/absolute/build/boundary-value-ports
iverilog -g2012 -DPYC_BOUNDARY_FOUR_STATE -s tb -o "$build/four-state.vvp" \
  /absolute/pycircuit/install/include/verilog/*.v \
  "$build/verilog/design_top.sv" \
  "$build/verilog/sources/example_boundary_value_ports/boundary_value_ports.v" \
  examples/boundary_value_ports/rtl_tb.sv
vvp "$build/four-state.vvp"
```

## Local cost comparison

The bench shrinks from 160 lines / 6,693 bytes to
65 lines / 1,915 bytes. These measurements use the same checkout-built
toolchain and Release settings on one machine. Stage and first-run costs are
single serial observations; warm medians use three alternating full-length pairs,
each in a fresh process. These brief runs do not establish a universal runtime
speedup or explain the causes of measured changes.

| Measurement | Expressions | Scenario Table |
| --- | ---: | ---: |
| Compile bench | 0.225669 s | 0.261893 s |
| Link system | 0.561205 s | 0.429350 s |
| Emit C++ | 0.507678 s | 0.417985 s |
| Emit Verilog | 0.397033 s | 0.391458 s |
| Build C++ | 1.617599 s | 1.719266 s |
| Build Verilog | 2.953362 s | 3.330857 s |
| First native, one worker | 0.545809 s | 0.428126 s |
| First native, two workers | 0.005577 s | 0.006640 s |
| First RTL | 0.253083 s | 0.297809 s |
| Warm native one-worker median | 0.007612 s | 0.007666 s |
| Warm native one-worker peak RSS median | 1.59 MiB | 1.61 MiB |
| Warm native two-worker median | 0.007788 s | 0.007710 s |
| Warm native two-worker peak RSS median | 1.66 MiB | 1.67 MiB |
| Warm RTL median | 0.006983 s | 0.008379 s |
| Warm RTL peak RSS median | 1.62 MiB | 1.64 MiB |

| Artifact size | Expressions | Scenario Table |
| --- | ---: | ---: |
| Final IR | 590,976 bytes | 284,071 bytes |
| Actual bench.hpp | 109,045 bytes | 72,461 bytes |
| Actual bench.v | 18,436 bytes | 12,628 bytes |
| All generated C++ code | 143,423 bytes | 107,675 bytes |
| All generated RTL code | 29,826 bytes | 24,271 bytes |
| C++ inventory including metadata | 150,727 bytes | 115,536 bytes |
| RTL inventory including metadata | 37,209 bytes | 32,211 bytes |

The measured RTL build increases by 12.78% in this single observation.
Warm runtime increases: native one-worker median +0.71% (2/3 paired runs slower); RTL median +19.99% (2/3 paired runs slower).
The tables retain every measured stage and first-run cost, including regressions.
Smaller source, IR and generated code do not establish faster build or runtime
performance. Full-catalog/nightly and cross-platform acceptance remain separate.

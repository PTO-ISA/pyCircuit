# Three combinational incrementers

Three connected instances of one reusable incrementer form a combinational
chain. Each adds one modulo 256; no registers or clocks are introduced. The result is
`(x + 3) & 255` in the same Work epoch. Every call returns an
`IncrementResult(y: ac.u8)` value; its field connects to the following call.
The root input is `ac.u8`, and its physical `result` packet retains that same
nominal result type. Each `ac.u8` addition wraps at eight bits.

```sh
export CMAKE_BUILD_TYPE=Release
cmake -S examples/hier_modules -B /absolute/build/hier-modules -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/hier-modules --parallel 4
ctest --test-dir /absolute/build/hier-modules --output-on-failure --no-tests=error
```

The original `x=1` to `y=4` case, rollover at each section and consecutive
samples without register delay are checked by independent native/RTL oracles.
Both worker settings execute the real generated modules. The dependency chain
requires ordered Work; this example does not claim parallel speedup.

Each stage uses the existing fixed-width bits Add lowering. No new
frontend operation, pass or backend special case is required. Execution and
the native and RTL tests bind the behavior to the generated artifacts.
See [the generated-output guide](GENERATED.md) for actual source-owned IR,
C++ and RTL from the public flow.

## Scenario bench

`bench.py` exports `example_hier_modules.bench.ExerciseHierModules`. Its immutable
Scenario Table contains 12 original input/expected pairs plus a separate all-zero tail row. Each row pairs the
original inputs with the independent expected values; constructing this data
adds no persistent state or clock cycle.

The full-width index selects `phase` below 12 and row 12 otherwise. The zero expected tail preserves the original disabled-check fallback; zero input still produces DUT output 3. The three incrementers remain combinational within the same epoch.

The original DUT calls, u64 phase initialization and wrapping advance, guards,
assertions, messages, logs and check-before-advance order remain unchanged.
Independent source proof verifies all active values, the complete known-u64
fallback and the remaining source structure. Arbitrary X/Z phase equivalence
is not claimed; the closed phase retains its known initialization and update.

Both source versions pass the existing module and system CTests. Thirty complete
first/warm/public system traces preserve all 24 observations and a successful
terminal result at 24 sampling epochs, including workers 1/2 and RTL. Only
changed AST positions are normalized in comparisons. This system covers the
original 12-cycle known-input scenario; physical oracle coverage remains separate.

The module checks 12 known samples and four native unknown/recovery cases. The separate original Icarus macro repeats its four unknown/recovery cases; those tasks do not print FOUR markers.

Use fresh build directories and export `CMAKE_BUILD_TYPE=Release` before
configuring and running tests. The public runner creates nested CMake builds,
which do not inherit the outer cache. Cycle counts, configuration and the
180-second command budgets remain unchanged. Complete source-import and
transformed artifacts match published source units; public and measured final
IR and generated outputs are byte-identical.

```sh
export CMAKE_BUILD_TYPE=Release
pycircuit run examples/hier_modules --target cpp --cycles 12
pycircuit run examples/hier_modules --target verilog --cycles 12
```

The Icarus macro is a separate check using the original testbench and generated
module RTL. After the standalone build above, run:

```sh
build=/absolute/build/hier-modules
iverilog -g2012 -DPYC_HIER_FOUR_STATE -s tb -o "$build/four-state.vvp" \
  /absolute/pycircuit/install/include/verilog/*.v \
  "$build/verilog/design_top.sv" \
  "$build/verilog/sources/example_hier_modules/hier_modules.v" \
  examples/hier_modules/rtl_tb.sv
vvp "$build/four-state.vvp"
```

## Local cost comparison

The bench shrinks from 124 lines / 4,020 bytes to
61 lines / 1,437 bytes. These measurements use the same checkout-built
toolchain and Release settings on one machine. Stage and first-run costs are
single serial observations; warm medians use three alternating full-length pairs,
each in a fresh process. These brief runs do not establish a universal runtime
speedup or explain the causes of measured changes.

| Measurement | Expressions | Scenario Table |
| --- | ---: | ---: |
| Compile bench | 0.194765 s | 0.245538 s |
| Link system | 0.333602 s | 0.373064 s |
| Emit C++ | 0.316556 s | 0.379390 s |
| Emit Verilog | 0.311967 s | 0.356527 s |
| Build C++ | 1.228004 s | 1.470105 s |
| Build Verilog | 2.649419 s | 3.281842 s |
| First native, one worker | 0.302743 s | 0.323885 s |
| First native, two workers | 0.003997 s | 0.008693 s |
| First RTL | 0.241495 s | 0.311839 s |
| Warm native one-worker median | 0.007825 s | 0.007714 s |
| Warm native one-worker peak RSS median | 1.61 MiB | 1.61 MiB |
| Warm native two-worker median | 0.007697 s | 0.007437 s |
| Warm native two-worker peak RSS median | 1.67 MiB | 1.67 MiB |
| Warm RTL median | 0.008326 s | 0.007078 s |
| Warm RTL peak RSS median | 1.64 MiB | 1.61 MiB |

| Artifact size | Expressions | Scenario Table |
| --- | ---: | ---: |
| Final IR | 387,269 bytes | 189,292 bytes |
| Actual bench.hpp | 78,667 bytes | 51,747 bytes |
| Actual bench.v | 14,391 bytes | 10,102 bytes |
| All generated C++ code | 98,193 bytes | 72,017 bytes |
| All generated RTL code | 22,145 bytes | 18,086 bytes |
| C++ inventory including metadata | 104,064 bytes | 78,429 bytes |
| RTL inventory including metadata | 28,143 bytes | 24,625 bytes |

The measured RTL build increases by 23.87% in this single observation.
The tables retain every measured stage and first-run cost, including regressions.
Smaller source, IR and generated code do not establish faster build or runtime
performance. Full-catalog/nightly and cross-platform acceptance remain separate.

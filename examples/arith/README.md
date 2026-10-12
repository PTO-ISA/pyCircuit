# Arithmetic example

This module has two 19-bit inputs,
a combinational 19-bit modular sum, a 16-bit lane mask of 65535 and an 8-bit
accumulator width output of 19. The original fixed configuration has eight
16-bit lanes, hence a 19-bit accumulator. No register or clock is introduced.
The source uses `import pycircuit as ac`, `ac.u19` inputs and a nominal
`ArithResult`. Unsigned bits addition wraps at 19 bits. Its ordered fields are
`sum: ac.u19`, `lane_mask: ac.u16 = 65535` and `acc_width: ac.u8 = 19`;
`ArithResult(sum=a + b)` supplies the sum and uses the two declared defaults.
The generated interface carries this value in one physical `result` packet.

With an installed pyCircuit package:

```sh
export CMAKE_BUILD_TYPE=Release
cmake -S examples/arith -B /absolute/build/arith -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/arith --parallel 4
ctest --test-dir /absolute/build/arith --output-on-failure --no-tests=error
```

The per-design driver and RTL testbench assert independent expected values,
including the original `(1, 2) -> 3` case and overflow boundaries. The shared
helper compares the Work samples from workers 1/2 and RTL. See [the generated-output guide](GENERATED.md) for
the source-owned IR, C++ and RTL produced by the public flow.

## Scenario bench

`bench.py` exports `example_arith.bench.ExerciseArith`. Its immutable
Scenario Table contains 10 original input/sum triples plus a separate all-zero tail row. Each row pairs the
original inputs with the independent expected values; constructing this data
adds no persistent state or clock cycle.

All three fields retain 19-bit widths and the full-width index selects `phase` below 10 and row 10 otherwise. The `lane_mask == 65535` and `acc_width == 19` assertions remain separate. Expected sums are retained literals, not recomputed from the inputs.

The original DUT calls, u64 phase initialization and wrapping advance, guards,
assertions, messages, logs and check-before-advance order remain unchanged.
Independent source proof verifies all active values, the complete known-u64
fallback and the remaining source structure. Arbitrary X/Z phase equivalence
is not claimed; the closed phase retains its known initialization and update.

Both source versions pass the existing module and system CTests. Thirty complete
first/warm/public system traces preserve all 20 observations and a successful
terminal result at 20 sampling epochs, including workers 1/2 and RTL. Only
changed AST positions are normalized in comparisons. This system covers the
original 10-cycle known-input scenario; physical oracle coverage remains separate.

The module checks the ten original known input rows and overflow cases on native workers 1/2 and Verilator. This example has no dedicated four-state macro; no new four-state result is claimed.

Use fresh build directories and export `CMAKE_BUILD_TYPE=Release` before
configuring and running tests. The public runner creates nested CMake builds,
which do not inherit the outer cache. Cycle counts, configuration and the
180-second command budgets remain unchanged. Complete source-import and
transformed artifacts match published source units; public and measured final
IR and generated outputs are byte-identical.

```sh
export CMAKE_BUILD_TYPE=Release
pycircuit run examples/arith --target cpp --cycles 10
pycircuit run examples/arith --target verilog --cycles 10
```

## Local cost comparison

The bench shrinks from 139 lines / 4,036 bytes to
63 lines / 1,573 bytes. These measurements use the same checkout-built
toolchain and Release settings on one machine. Stage and first-run costs are
single serial observations; warm medians use three alternating full-length pairs,
each in a fresh process. These brief runs do not establish a universal runtime
speedup or explain the causes of measured changes.

| Measurement | Expressions | Scenario Table |
| --- | ---: | ---: |
| Compile bench | 0.235676 s | 0.258944 s |
| Link system | 0.392624 s | 0.399637 s |
| Emit C++ | 0.366383 s | 0.403096 s |
| Emit Verilog | 0.322398 s | 0.380548 s |
| Build C++ | 1.817600 s | 1.499473 s |
| Build Verilog | 2.794652 s | 3.239343 s |
| First native, one worker | 0.282451 s | 0.295209 s |
| First native, two workers | 0.004378 s | 0.007933 s |
| First RTL | 0.241311 s | 0.307727 s |
| Warm native one-worker median | 0.007676 s | 0.007680 s |
| Warm native one-worker peak RSS median | 1.61 MiB | 1.61 MiB |
| Warm native two-worker median | 0.008384 s | 0.007569 s |
| Warm native two-worker peak RSS median | 1.67 MiB | 1.67 MiB |
| Warm RTL median | 0.007935 s | 0.008271 s |
| Warm RTL peak RSS median | 1.62 MiB | 1.58 MiB |

| Artifact size | Expressions | Scenario Table |
| --- | ---: | ---: |
| Final IR | 467,029 bytes | 227,893 bytes |
| Actual bench.hpp | 100,410 bytes | 64,907 bytes |
| Actual bench.v | 18,783 bytes | 11,710 bytes |
| All generated C++ code | 113,682 bytes | 78,920 bytes |
| All generated RTL code | 24,961 bytes | 18,130 bytes |
| C++ inventory including metadata | 117,822 bytes | 83,588 bytes |
| RTL inventory including metadata | 29,270 bytes | 22,967 bytes |

The measured RTL build increases by 15.91% in this single observation.
Warm runtime increases: native one-worker median +0.05% (3/3 paired runs slower); RTL median +4.22% (2/3 paired runs slower).
The tables retain every measured stage and first-run cost, including regressions.
Smaller source, IR and generated code do not establish faster build or runtime
performance. Full-catalog/nightly and cross-platform acceptance remain separate.

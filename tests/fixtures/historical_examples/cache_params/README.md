# Fixed cache address fields

[Generated MLIR, C++ and RTL](GENERATED.md)

This fixed configuration has four ways, 64 sets, 64-byte lines, 40-bit
addresses and 64-bit data. Six offset bits and six index bits leave a 28-bit
tag. The module exposes `tag=addr[12:40]` and the nine-bit constants
`line_words=8` and `tag_bits=28`. It has no public configuration parameters,
state or clock inputs.

```sh
export CMAKE_BUILD_TYPE=Release
cmake -S examples/cache_params -B /absolute/build/cache-params -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/cache-params --parallel 4
ctest --test-dir /absolute/build/cache-params --output-on-failure --no-tests=error
```

The source uses `addr: ac.u40` and returns a typed `CacheResult`. Its nine-bit
constants are declared struct defaults, so the constructor supplies only the
tag. Both backends consume the same existing bit-extract operation. Low
twelve X/Z bits are discarded; retained X and Z remain at their tag positions.
The original zero-address case, both sides of the bit12 boundary, high bit39,
maximum address, consecutive tags and all three outputs are checked by
independent native/RTL oracles. No cache-specific compiler rule or source
helper execution is needed.

## Scenario bench

`bench.py` exports `example_cache_params.bench.ExerciseCacheParams`. Its immutable
Scenario Table contains 14 original address/tag pairs. Each row pairs the
original inputs with the independent expected values; constructing this data
adds no persistent state or clock cycle.

Row 13 is already zero and also supplies the original tail. The full-width index selects `phase` below 14 and row 13 otherwise. Addresses remain 40 bits and tags 28 bits; the `line_words == 8` and `tag_bits == 28` assertions remain separate.

The original DUT calls, u64 phase initialization and wrapping advance, guards,
assertions, messages, logs and check-before-advance order remain unchanged.
Independent source proof verifies all active values, the complete known-u64
fallback and the remaining source structure. Arbitrary X/Z phase equivalence
is not claimed; the closed phase retains its known initialization and update.

Both source versions pass the existing module and system CTests. Thirty complete
first/warm/public system traces preserve all 28 observations and a successful
terminal result at 28 sampling epochs, including workers 1/2 and RTL. Only
changed AST positions are normalized in comparisons. This system covers the
original 14-cycle known-input scenario; physical oracle coverage remains separate.

The module checks 14 known samples and seven exact value/known/Z cases with immediate recovery. Seven raw MASK records agree across native workers 1/2 and the original Icarus macro. The shared known-value verifier does not compare MASK records; this comparison is separate.

Use fresh build directories and export `CMAKE_BUILD_TYPE=Release` before
configuring and running tests. The public runner creates nested CMake builds,
which do not inherit the outer cache. Cycle counts, configuration and the
180-second command budgets remain unchanged. Complete source-import and
transformed artifacts match published source units; public and measured final
IR and generated outputs are byte-identical.

```sh
export CMAKE_BUILD_TYPE=Release
pycircuit run examples/cache_params --target cpp --cycles 14
pycircuit run examples/cache_params --target verilog --cycles 14
```

The Icarus macro is a separate check using the original testbench and generated
module RTL. After the standalone build above, run:

```sh
build=/absolute/build/cache-params
iverilog -g2012 -DPYC_CACHE_FOUR_STATE -s tb -o "$build/four-state.vvp" \
  /absolute/pycircuit/install/include/verilog/*.v \
  "$build/verilog/design_top.sv" \
  "$build/verilog/sources/example_cache_params/cache_params.v" \
  examples/cache_params/rtl_tb.sv
vvp "$build/four-state.vvp"
```

## Local cost comparison

The bench shrinks from 142 lines / 5,280 bytes to
64 lines / 1,762 bytes. These measurements use the same checkout-built
toolchain and Release settings on one machine. Stage and first-run costs are
single serial observations; warm medians use three alternating full-length pairs,
each in a fresh process. These brief runs do not establish a universal runtime
speedup or explain the causes of measured changes.

| Measurement | Expressions | Scenario Table |
| --- | ---: | ---: |
| Compile bench | 0.288477 s | 0.258491 s |
| Link system | 0.369136 s | 0.387643 s |
| Emit C++ | 0.345616 s | 0.372666 s |
| Emit Verilog | 0.330913 s | 0.373473 s |
| Build C++ | 1.342678 s | 1.482731 s |
| Build Verilog | 2.486826 s | 3.225194 s |
| First native, one worker | 0.363938 s | 0.300962 s |
| First native, two workers | 0.005799 s | 0.007902 s |
| First RTL | 0.303430 s | 0.331020 s |
| Warm native one-worker median | 0.009642 s | 0.007656 s |
| Warm native one-worker peak RSS median | 1.61 MiB | 1.61 MiB |
| Warm native two-worker median | 0.007693 s | 0.007771 s |
| Warm native two-worker peak RSS median | 1.67 MiB | 1.67 MiB |
| Warm RTL median | 0.007736 s | 0.007931 s |
| Warm RTL peak RSS median | 1.61 MiB | 1.62 MiB |

| Artifact size | Expressions | Scenario Table |
| --- | ---: | ---: |
| Final IR | 489,083 bytes | 215,721 bytes |
| Actual bench.hpp | 96,777 bytes | 59,833 bytes |
| Actual bench.v | 18,125 bytes | 11,690 bytes |
| All generated C++ code | 110,179 bytes | 73,991 bytes |
| All generated RTL code | 24,380 bytes | 18,182 bytes |
| C++ inventory including metadata | 114,549 bytes | 78,902 bytes |
| RTL inventory including metadata | 28,877 bytes | 23,220 bytes |

The measured RTL build increases by 29.69% in this single observation.
Warm runtime increases: native two-worker median +1.02% (2/3 paired runs slower); RTL median +2.52% (2/3 paired runs slower).
The tables retain every measured stage and first-run cost, including regressions.
Smaller source, IR and generated code do not establish faster build or runtime
performance. Full-catalog/nightly and cross-platform acceptance remain separate.

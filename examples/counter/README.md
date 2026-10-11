# Enabled eight-bit counter

[Generated MLIR, C++ and RTL](GENERATED.md)

```sh
cmake -S examples/counter -B /absolute/build/counter -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/counter --parallel 4
ctest --test-dir /absolute/build/counter --output-on-failure --no-tests=error
```

The source declares an ordinary `count: bits[8] = 0` and returns a typed
`CounterResult`. The rule saves the result before assigning its modular increment
or unchanged value. This unconditional data selection preserves the original
unknown-enable merge; a guarded write would instead make enable a storage
control. Clock/reset are absent from Python and bound by the host runner.
One Work samples old Q; Xfer commits the
active edge. The next Work observes the new value, with no added Eval stage.

The per-design driver and RTL testbench validate original values 1..5, a real
256-edge wrap, enable hold, repeated clock levels and reset priority. The native
DUT also checks X/Z enable and partially unknown state with workers 1/2. Known
whole-counter RTL runs with Verilator. The `PYC_COUNTER_FOUR_STATE` testbench
mode also checks the complete generated DUT and its standard storage with
Icarus: Q0/Q3 with enable X/Z, partial-X hold and reset recovery.

## Scenario bench

`bench.py` exports `example_counter.bench.ExerciseCounter`. It retains the
complete 256-enabled-edge wrap and subsequent enable/hold sequence, with
263 managed cycles and 526 logged observations. Only the expanded
`expected_count` expression changes; inputs, DUT calls, assertions, messages,
logs and check-before-advance registration remain unchanged.

The expected value uses the low eight bits while the complete 64-bit phase is
below 256, followed by the original hold intervals:

| Phase | Expected count |
| --- | ---: |
| 0–255 | Low eight bits of phase |
| 256–257 | 0 |
| 258 | 1 |
| 259–261 | 2 |
| 262 and above | 3 |

The phase still starts at zero and increments modulo 2^64. No Table, helper,
import or public API is added. Independent source proof preserves all 526
active enable/count values and proves the complete known-u64 domain through
original comparison partitions, prefix singletons and constant tails. It also
checks the exact remaining source AST. This does not claim arbitrary X/Z
internal-phase equivalence; the phase remains known through initialization
and increment.

Both original and compact versions pass the two existing standalone tests.
Twenty complete native/RTL traces agree on all 526 observations and the terminal
result after excluding only changed source-position metadata. The unchanged
physical drivers retain 529 known Work samples and their original eight native
unknown-selector cases per invocation. Explicit Icarus runs retain all four
original X/Z selector cases, hold/reset/recovery checks and complete output;
the shared CTest verifier does not invoke that macro automatically. System
coverage does not replace physical clock/reset or four-state coverage.

See [generated excerpts and verification inputs](GENERATED.md).

## Local cost comparison

The source shrinks from 1,076 lines / 45,541 bytes to
52 lines / 1,257 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded expressions | Compact expression |
| --- | ---: | ---: |
| Compile bench | 0.737 s | 0.115 s |
| Link system | 1.137 s | 0.178 s |
| Emit C++ | 1.027 s | 0.188 s |
| Emit Verilog | 0.871 s | 0.169 s |
| Build C++ | 0.788 s | 0.697 s |
| Build Verilog | 1.374 s | 1.636 s |
| First measured full C++ run | 0.601 s | 0.424 s |
| First measured full Verilog run | 0.289 s | 0.452 s |
| Warm C++ median, one worker | 0.171078 s | 0.014542 s |
| Warm Verilog median | 0.008017 s | 0.008012 s |
| Warm native peak RSS median | 2.48 MiB | 1.81 MiB |

| Artifact size | Expanded expressions | Compact expression |
| --- | ---: | ---: |
| Final IR | 4,744,012 bytes | 225,239 bytes |
| Generated C++ total | 901,849 bytes | 77,273 bytes |
| Generated Verilog total | 195,051 bytes | 20,777 bytes |

Recorded first-run regressions: RTL 0.289 → 0.452 s.
They remain separate from warm medians; one observation does not identify a cause.

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/counter --target cpp --cycles 263
pycircuit run examples/counter --target verilog --cycles 263
```

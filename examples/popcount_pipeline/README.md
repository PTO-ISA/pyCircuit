# Buffered population count

`Item` retains the complete 17-bit token: unsigned `value:u13` followed by
`count:u4`. The pure typed `count_item` rule copies the record and replaces only
its count. For known value `v`, the new count is the sum of its 13 individual
bits, in the range 0 through 13. The packed output token is `(v << 4) | count`,
regardless of the incoming count field. `Result` packs ready and valid above
the complete token, for 19 bits total.

There are exactly two depth-one, latency-one queue owners, with explicit
`downstream_pop` readiness, old committed head reads and no empty bypass. Their
total capacity is two complete tokens. The sink adds no storage. E0 captures
an input, E1 commits the transformed old input head to the result queue, and
E2 is the earliest external retirement. Full simultaneous replacement must
retain ordering. Held clock levels do not transfer tokens; a rising reset
drops both queued tokens. Clock/reset levels are supplied by the host driver.

The copied value field must preserve every value/known/Z bit. Any X/Z bit in
value produces four X count bits with no Z; the hidden value plane of that
computed unknown count is unspecified. Uncertainty in the old count alone is
overwritten and cannot contaminate the new count. Zero and uncertain data are
valid tokens.

## Build and verification

```sh
cmake -S examples/popcount_pipeline -B /absolute/build/popcount_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/popcount_pipeline --parallel 4
ctest --test-dir /absolute/build/popcount_pipeline --output-on-failure --no-tests=error
```

The installed shared CMake helper independently compiles the source, links its
explicit closure and emits both targets from one verified final artifact. The
same module and four-state gates are registered in the aggregate examples build. Build products
belong outside the source tree.

Native workers 1/2 and Verilator check 16,585 known Work samples, including all
8192 values with varied old counts and all 16 prior counts for five focused
values. Genuine Icarus agrees with the known trace and 213 four-state samples.
Native tests cover 52 per-bit value uncertainty cases, 16 overwritten-count
uncertainty cases and eight dense latent variants; copied value planes are
strictly checked, including latent values behind X/Z. Computed unknown count
value planes remain unconstrained. Both paths check recovery and uncertain
stalled tokens using two old-state slots and a separate commit-qualified ledger.

Known history is 8279 accepted / 8275 retired / 4 reset-dropped; four-state
history is 88 / 84 / 4. Both finish empty and reach capacity two. E0/E1/E2,
full replacement, changed offers under held clocks and occupied reset are
checked. Runner limits are finite at 20,000 ticks. Final IR checks exactly two
D1/L1 queues with 17-bit payloads and the reviewed policies; no extra instances
or collections are introduced.

## Scenario bench

`bench.py` stores all 184 original scenarios in an immutable typed Table. Its
8-field, 38-bit `Stimulus` preserves the original schema, field order and all
input/expected payloads. Row 184 is all zero, retaining the helper fallback
for every known u16 phase from 184 through 65,535. This fallback is distinct
from the system counter, which holds at 183. The run retains 184 cycles.

The original value13 and count4 inputs remain intact. Old counts retain the
complete 0–15 range, including overwritten 14/15 values; the helper does not
precompute the DUT result or clamp old counts to 13. All data assertions remain
unmasked, preserving invalid empty-output expectations as well as real tokens.

The complete advance and system bodies are unchanged, including unconditional
assertions, observation order and advance-before-check/exercise registration.
Independent source checks compare all 524,288 field values across the known-u16
helper domain. Twenty complete native and RTL traces match all 1,840 observations
and the terminal result after excluding only changed source-position metadata,
including native workers 1 and 2. Baseline and compact versions each pass all
three standalone tests. See the module [excerpts and receipt](GENERATED.md).

Original physical tests retain 16,585 known and 213 four-state Work samples,
with the reset, held-clock and conservation checks described above. The
registered four-state test already executes Icarus. Regular-clock systems do not
replace physical histories, and known-u16 source proof does not establish
arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 1,511 lines / 38,121 bytes to
253 lines / 26,827 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 1.079 s | 0.790 s |
| Link system | 1.727 s | 1.359 s |
| Emit C++ | 1.522 s | 1.235 s |
| Emit Verilog | 1.278 s | 1.003 s |
| Build C++ | 1.140 s | 0.934 s |
| Build Verilog | 1.567 s | 1.510 s |
| First measured full C++ run | 0.513 s | 0.301 s |
| First measured full Verilog run | 1.283 s | 0.259 s |
| Warm C++ median, one worker | 0.210809 s | 0.024160 s |
| Warm Verilog median | 0.020774 s | 0.020168 s |
| Warm native peak RSS median | 3.55 MiB | 2.72 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 7,041,812 bytes | 5,378,782 bytes |
| Generated C++ total | 1,814,010 bytes | 1,514,420 bytes |
| Generated Verilog total | 398,166 bytes | 305,313 bytes |

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/popcount_pipeline --target cpp --cycles 184
pycircuit run examples/popcount_pipeline --target verilog --cycles 184
```

# Two-stage unsigned increment pipeline

The original spelling `int` resolves to unsigned fixed 64-bit hardware, so
this source uses `ac.u64`. Its inline pure expression increments each token
modulo 2^64: UINT64_MAX becomes zero, and 2^63-1 becomes 2^63. It preserves
the full 64-bit payload. `QueueResult` packs data at bits 0 through 63, valid
at bit 64 and ready at bit 65, for 66 bits total.

Two explicit queues each have depth 2 and availability latency 1. They hold
four complete tokens in total, use old committed head reads, have no empty
bypass and declare `downstream_pop` readiness. The sink adds no storage.
E0 captures an input, E1 commits its incremented old head into the result
queue, and E2 is the earliest external retirement. Depth 2 adds capacity,
without adding a mandatory latency stage.

For old input/result occupancies n0/n1, result readiness is `n1 < 2` or an
old result-head pop. An old input head advances only when that result queue
is ready. Input readiness is `n0 < 2` or an old input-head advance. Full
simultaneous replacement retains FIFO order and occupancy through both
depth-two pointer wraps. Held clock levels do not commit despite changed
offers/take; rising reset clears up to four tokens. The host drives physical
clock/reset levels; the Python source adds no clock or transaction proposal.

Any X/Z bit in an input produces 64 X data bits with no Z. The hidden value
plane of that computed unknown is unspecified. Zero and unknown data remain
valid tokens.

## Build route

Use the accepted installed toolchain and the public example CMake helper,
which compiles the source independently, links its explicit unit closure and
emits both targets from one verified final artifact. Run from the repository
root:

```sh
cmake -S examples/queue_pipeline -B /absolute/build/queue_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/queue_pipeline --parallel 4
ctest --test-dir /absolute/build/queue_pipeline --output-on-failure --no-tests=error
```

Keep build artifacts outside the source tree. The module and four-state gates also run through the
aggregate examples build.

## Verification

Actual generated native workers 1/2 and Verilator agree on 483 known Work
samples. Genuine full-DUT Icarus agrees with these and 607 four-state Work
samples. The independent oracle uses 64-bit ripple carry/borrow and two
old-state depth-two logical deques, with a separate actual-DUT transaction
ledger. 202 known stimulus tokens and 264 native latent X/Z variants
cover the arithmetic; no exhaustive 64-bit or 128-bit claim is made.

Every carry length 0 through 64, zero/MAX/high-bit/alternating values and
full-width deterministic mixed values are exercised. Four-state tests cover
every input bit as X and Z with both latent values, all-X/all-Z and dense mixed
patterns. Computed unknown data must be all X with zero Z; its hidden value is
not asserted.

Both streams exercise E0/E1/E2, full four-token stalls and replacements,
multiple wraps of both queue pointers, active held clocks with changed offers
and reset, two rising resets dropping four tokens each, drain and recovery.
Known history is 223 accepted / 215 retired / 8 reset-dropped; four-state 285 / 277 / 8. Every run ends empty, reaches peak occupancy four
and conserves all accepted tokens. Runner limits are 12,000 ticks.

## Scenario bench

`bench.py` stores all 158 original scenarios in an immutable typed Table. Its
6-field, 132-bit `Stimulus` preserves field order, exact values and complete
input/expected payloads. Row 158 is all zero, retaining the helper fallback
for every known u16 phase from 158 through 65,535. That fallback is distinct
from the system counter, which holds at 157. The run still lasts 158 cycles.

Both D2 queues remain independent owners, with four-token capacity, one u64
increment and E2 earliest retirement. Full-width values and empty/invalid data
expectations remain literal; depth is not converted into another latency stage.

The complete advance and system bodies remain unchanged, including ordered
arguments, all unmasked assertions/messages, log order and advance-before-check
registration. Independent source checks compare all 393,216 field values across
the known-u16 domain. Twenty complete native and RTL traces match all 1,264
observations and the terminal result after excluding only changed source-position
metadata, including native workers 1 and 2. Baseline and compact versions each
pass all 3 standalone tests. See the module [excerpts and receipt](GENERATED.md).

The original physical tests retain 483 known Work samples and 607 four-state Work samples.
These are complete run counts, not just minimum assertions. Registered four-state
helpers already execute the existing Icarus paths, including their original
histories and negative probes where present.

Regular-clock systems do not replace physical reset/held-clock histories, and
known-u16 source proof does not establish arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 1,018 lines / 25,060 bytes to
216 lines / 17,681 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 2.330 s | 0.508 s |
| Link system | 4.052 s | 0.893 s |
| Emit C++ | 3.000 s | 0.827 s |
| Emit Verilog | 2.743 s | 0.686 s |
| Build C++ | 0.950 s | 0.821 s |
| Build Verilog | 1.489 s | 1.392 s |
| First measured full C++ run | 1.406 s | 0.690 s |
| First measured full Verilog run | 0.694 s | 0.455 s |
| Warm C++ median, one worker | 0.121323 s | 0.015344 s |
| Warm Verilog median | 0.014826 s | 0.014390 s |
| Warm native peak RSS median | 2.95 MiB | 2.33 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 19,752,996 bytes | 3,445,645 bytes |
| Generated C++ total | 1,237,101 bytes | 980,336 bytes |
| Generated Verilog total | 275,405 bytes | 208,658 bytes |

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/queue_pipeline --target cpp --cycles 158
pycircuit run examples/queue_pipeline --target verilog --cycles 158
```

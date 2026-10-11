# Scalar bitfield permutation

## Payload behavior

The input and output are 32-bit words. The original operation swapped low17 and
high15, then replaced the result's low3 with the original word's mode bits.
The equivalent expression is `ac.concat(word[:17], word[20:32], word[:3])`.
Output bits31:15 copy input16:0; output14:3 copy input31:20; output2:0 copy
input2:0 again. Input17:19 are discarded and input0:2 are duplicated. This is
not a rotate. All selected value/known/Z bits are transported exactly.

The pure, single-use permutation is an ordinary module-local expression between
the two queues. The old rule-result queue is retained explicitly. An initial
annotated scalar-rule probe was rejected by the existing return-annotation
contract; this rewrite does not extend that contract or wrap scalar values in
an artificial state owner.

## Original timing

Exactly two depth1/latency1 queues preserve the inferred input queue and
rule-result queue. Both explicitly select `downstream_pop` for original full
replacement. Returning the result adds no third queue; no empty bypass exists.
E0 captures input, E1 commits the result stage, and E2 is earliest consumption.
All Work samples use old state. Held clocks do not transfer; stalls fill at most
two slots; rising reset clears both stages. Python leaves clock/reset/proposal
wires hidden. The host drives the physical pins through the existing typed DUT.

## Build and verification

```sh
cmake -S examples/bitfield_scalar_pipeline -B /absolute/build/bitfield_scalar_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/bitfield_scalar_pipeline --parallel 4
ctest --test-dir /absolute/build/bitfield_scalar_pipeline --output-on-failure --no-tests=error
```

The module and four-state CTest gates run generated native workers 1/2, Verilator and genuine
Icarus. They compare 107 known Work rows and 179 native/Icarus four-state
Work rows. Known one-hot vectors and isolated X/Z at every input position,
plus dense patterns with nonzero latent values, check all three native planes.
Icarus checks visible X/Z placement. The two-slot oracle includes startup,
full replacement, stalls, changing rejected offers, active held high/low clocks,
bubbles, drain and reset. All runners have explicit finite limits.

Known accepted/retired/reset-dropped counts are 42/38/4; four-state counts are
78/74/4. Both histories finish empty, with peak occupancy two. Final IR checks
confirm two D1 queues, availability latency1, combinational head-read latency0,
explicit downstream replacement, no empty flow and no additional state owners.

## Scenario bench

`bench.py` stores all 158 original scenarios in an immutable typed Table. Its
6-field, 68-bit `Stimulus` preserves field order, exact values and complete
input/expected payloads. Row 158 is all zero, retaining the helper fallback
for every known u16 phase from 158 through 65,535. That fallback is distinct
from the system counter, which holds at 157. The run still lasts 158 cycles.

All input and expected bits remain exact. The unchanged DUT's concat duplicates
input bits 0–2 and discards 17–19; the bench does not replace that permutation
with a rotate, arithmetic approximation or recomputed expectation.

The complete advance and system bodies remain unchanged, including ordered
arguments, all unmasked assertions/messages, log order and advance-before-check
registration. Independent source checks compare all 393,216 field values across
the known-u16 domain. Twenty complete native and RTL traces match all 1,264
observations and the terminal result after excluding only changed source-position
metadata, including native workers 1 and 2. Baseline and compact versions each
pass all 3 standalone tests. See the module [excerpts and receipt](GENERATED.md).

The original physical tests retain 107 known Work samples and 179 four-state Work samples.
These are complete run counts, not just minimum assertions. Registered four-state
helpers already execute the existing Icarus paths, including their original
histories and negative probes where present.

Regular-clock systems do not replace physical reset/held-clock histories, and
known-u16 source proof does not establish arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 995 lines / 23,216 bytes to
224 lines / 16,116 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 2.291 s | 0.498 s |
| Link system | 3.994 s | 0.892 s |
| Emit C++ | 3.014 s | 0.827 s |
| Emit Verilog | 2.720 s | 0.685 s |
| Build C++ | 0.972 s | 0.823 s |
| Build Verilog | 1.525 s | 1.389 s |
| First measured full C++ run | 0.562 s | 0.629 s |
| First measured full Verilog run | 0.718 s | 0.848 s |
| Warm C++ median, one worker | 0.119719 s | 0.016034 s |
| Warm Verilog median | 0.015197 s | 0.014676 s |
| Warm native peak RSS median | 2.97 MiB | 2.39 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 19,563,283 bytes | 3,467,014 bytes |
| Generated C++ total | 1,214,111 bytes | 971,041 bytes |
| Generated Verilog total | 269,654 bytes | 207,006 bytes |

Recorded first-run regressions: native 0.562 → 0.629 s, RTL 0.718 → 0.848 s.
They remain separate from warm medians; one observation does not identify a cause.

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/bitfield_scalar_pipeline --target cpp --cycles 158
pycircuit run examples/bitfield_scalar_pipeline --target verilog --cycles 158
```

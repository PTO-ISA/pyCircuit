# Atomic broadcast pipeline

An unsigned 64-bit source queue (D2) feeds two atomic broadcast destinations
(D1 each). Separate modular +1 and +2 transforms each feed a D1 result queue.
A left-priority merge feeds the D2 output queue. All six queues have latency
one and explicit `downstream_pop` readiness: eight payload slots total.
The implicit historical fanout queues remain explicit in this rewrite.

Each source token produces two transformed results, with FIFO order preserved
within each branch. The left branch wins when both merge inputs are available;
global source-order pairing at the output is not promised. For one token from
empty state, source birth E0, fanout birth E1 and both transform births E2 lead
to left merge birth E3 and right merge birth E4. Their earliest retirements are
E4 and E5 respectively. No queue permits empty bypass.

`BroadcastPipeline(valid, data, take)` returns `BroadcastResult` with ready at
bit 65, valid at 64 and data at 63:0. Python uses ordinary queues and fixed-bit
expressions. Existing common-IR analysis verifies all forward connections;
both backends consume the same final artifact. No broadcast primitive is added.

Work observes old heads; only committed rising edges move tokens. Rising reset
empties every queue; held and falling levels retain state. Raw fanout transport
preserves value/known/Z planes. Computed +1/+2 values containing any X/Z are
all-X with zero known/Z masks; latent computed value bits are unspecified.
Failed whole-system checking discards proposals and requires Reset before reuse.

## Build and verification

```sh
cmake -S examples/broadcast_pipeline -B /absolute/build/broadcast_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/broadcast_pipeline --parallel 4
ctest --test-dir /absolute/build/broadcast_pipeline --output-on-failure --no-tests=error
```

Independent tests must track both branch obligations per source identity,
asymmetric stalls, no partial fanout or duplicated results, priority merge,
capacity, old-head replacement, wrap, reset drops, clock holds and four-state
arithmetic. The installed helper runs actual generated native workers 1/2 and
RTL, with separate genuine Icarus four-state execution.

Standalone generated-DUT gates pass: native workers 1/2, Verilator and Icarus
agree on 1,885 known Work samples; native and genuine Icarus agree on 1,867
four-state samples. All 267 known and 264 raw input patterns are accepted.
Each branch's known history accepts 292 source obligations, retires 282 and
reset-drops ten; the four-state history accepts 289, retires 279 and drops ten.
Two full resets each drop five obligations per branch at eight occupied slots.

The independent oracle checks atomic delivery and the historical asymmetric-stall
duplication counterexample, with direct-owner discard/retry and discarded reset.
A terminal unknown sink-pop test checks failed-system discipline and mandatory
Reset. Internal source/fan payload copying follows the unchanged Q6 FIFO and
the verified same-width emitted connections. The public observations directly
check transformed results; they do not expose latent internal copy bits.

## Scenario bench

`bench.py` stores all 158 original scenarios in an immutable typed Table. Its
6-field, 132-bit `Stimulus` preserves field order, exact values and complete
input/expected payloads. Row 158 is all zero, retaining the helper fallback
for every known u16 phase from 158 through 65,535. That fallback is distinct
from the system counter, which holds at 157. The run still lasts 158 cycles.

Every input still produces separate +1 and +2 results after atomic publication,
with left-priority merging. The bench retains both obligations and priority order;
it does not substitute the fork's partial-delivery flags or pair outputs by input
order. All six queues and eight slots remain unchanged.

The complete advance and system bodies remain unchanged, including ordered
arguments, all unmasked assertions/messages, log order and advance-before-check
registration. Independent source checks compare all 393,216 field values across
the known-u16 domain. Twenty complete native and RTL traces match all 1,264
observations and the terminal result after excluding only changed source-position
metadata, including native workers 1 and 2. Baseline and compact versions each
pass all 3 standalone tests. See the module [excerpts and receipt](GENERATED.md).

The original physical tests retain 1,885 known Work samples and 1,867 four-state Work samples.
These are complete run counts, not just minimum assertions. Registered four-state
helpers already execute the existing Icarus paths, including their original
histories and negative probes where present.

Regular-clock systems do not replace physical reset/held-clock histories, and
known-u16 source proof does not establish arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 977 lines / 23,851 bytes to
222 lines / 16,895 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 2.297 s | 0.490 s |
| Link system | 3.977 s | 0.917 s |
| Emit C++ | 3.027 s | 0.861 s |
| Emit Verilog | 2.731 s | 0.709 s |
| Build C++ | 0.977 s | 0.878 s |
| Build Verilog | 1.510 s | 1.375 s |
| First measured full C++ run | 1.075 s | 0.506 s |
| First measured full Verilog run | 0.747 s | 0.487 s |
| Warm C++ median, one worker | 0.120706 s | 0.017145 s |
| Warm Verilog median | 0.015388 s | 0.015062 s |
| Warm native peak RSS median | 2.97 MiB | 2.39 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 19,476,359 bytes | 3,427,972 bytes |
| Generated C++ total | 1,266,947 bytes | 1,022,995 bytes |
| Generated Verilog total | 274,445 bytes | 210,800 bytes |

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/broadcast_pipeline --target cpp --cycles 158
pycircuit run examples/broadcast_pipeline --target verilog --cycles 158
```

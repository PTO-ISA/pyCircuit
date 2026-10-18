# Delayed increment pipeline

The issue #319 source migration is an unaccepted savepoint pending generated
C++/Verilog execution of compiler-owned queue admission. Named ingress and
egress handles retain the depth-two/latency-one ingress and depth-four/latency-three egress `downstream_pop` queues.
One registered `transfer` Rule pops the old ingress head, computes the payload
and pushes the egress atomically; the compiler supplies availability and capacity
guards. Raw `in_valid`/`in_data` and `out_ready` bindings retain the external
handshake, and `ready`/`valid`/`data` expose the queue boundary views. Test counts,
measurements and generated excerpts below describe the preceding implementation
and are not acceptance evidence for this migration.

The original `int` is unsigned fixed64 hardware. Each token increments modulo
2^64, including MAX to0, without narrowing. `LatencyResult` packs ready65,
valid64 and data63:0. Source clocks/reset and proposal state remain hidden.

## Storage and timing

There are exactly two queue owners: the input has depth2/latency1 and the result
has depth4/latency3. Both use explicit downstream-pop readiness and old head
reads with no empty bypass. Waiting result tokens reserve their existing slots;
total capacity is six complete tokens. The historical RTL's two extra result
FIFO stages inflated capacity to eight. The accepted Q6 contract follows the
declared source/native capacity and removes that defect.

An input captured on E0 can move through the pure increment on E1. Its result
becomes available after E3 Xfer and can first retire on E4. Work on a maturity
edge still observes the preceding state; a subsequent nonedge Work can see the
available token without committing a transfer. Valid is false and data packed
zero while the result head remains unavailable, even if the queue is occupied.

Ready uses capacity plus a real old-head pop. Full replacements retain old-head
ordering; held and falling clock levels do not age or transfer tokens. Rising
reset empties both queues. Any X/Z in the arithmetic operand produces an all-X
64-bit result with no Z and unspecified computed value plane. Zero and unknown
payloads are still tokens.

## Build

```sh
cmake -S examples/latency_pipeline -B /absolute/build/latency_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/latency_pipeline --parallel 4
ctest --test-dir /absolute/build/latency_pipeline --output-on-failure --no-tests=error
```

The installed shared helper uses independent source compile, explicit link and
both emissions from one verified final artifact. Build products stay outside
the source tree. The module and four-state gates also run in the aggregate examples build.

## Verification

Actual generated native workers1/2, known Verilator and genuine Icarus agree on
555 known Work samples and679 four-state samples. The independent reference uses
absolute birth/maturity edges and two logical deques, not the Runtime timestamp
algorithm. It checks every carry/wrap boundary and raw input uncertainty, with
exact arithmetic known/Z masks and no incidental computed-X value requirement.

Known history is229 accepted/217 retired/12 reset-dropped; four-state history
is291/279/12. Both reach six slots and drain fully, including two resets dropping
six tokens. E0/E1/E3/E4, full replacement, held/falling clocks and unavailable
packed-zero data are checked. Normal and raw runners have finite2000-tick limits.
A separate public-owner probe discards E3 maturity and retries the same edge;
a distinct SimExecutor failure probe checks unchanged epoch, invalid resumption,
and mandatory Reset before recovery. It does not claim failed-system retry.

## Scenario bench

`bench.py` stores all 158 original scenarios in an immutable typed Table. Its
6-field, 132-bit `Stimulus` preserves field order, exact values and complete
input/expected payloads. Row 158 is all zero, retaining the helper fallback
for every known u16 phase from 158 through 65,535. That fallback is distinct
from the system counter, which holds at 157. The run still lasts 158 cycles.

The D2/L1 input and D4/L3 result retain six total slots and E4 earliest retirement.
Waiting tokens reserve existing result slots. The bench preserves unavailable-head
zeros, maturity and drain observations rather than adding capacity or using E2.

The complete advance and system bodies remain unchanged, including ordered
arguments, all unmasked assertions/messages, log order and advance-before-check
registration. Independent source checks compare all 393,216 field values across
the known-u16 domain. Twenty complete native and RTL traces match all 1,264
observations and the terminal result after excluding only changed source-position
metadata, including native workers 1 and 2. Baseline and compact versions each
pass all 3 standalone tests. See the module [excerpts and receipt](GENERATED.md).

The original physical tests retain 555 known Work samples and 679 four-state Work samples.
These are complete run counts, not just minimum assertions. Registered four-state
helpers already execute the existing Icarus paths, including their original
histories and negative probes where present.

Regular-clock systems do not replace physical reset/held-clock histories, and
known-u16 source proof does not establish arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 1,024 lines / 25,195 bytes to
220 lines / 17,798 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 2.345 s | 0.502 s |
| Link system | 4.043 s | 0.903 s |
| Emit C++ | 3.048 s | 0.831 s |
| Emit Verilog | 2.754 s | 0.690 s |
| Build C++ | 0.970 s | 0.820 s |
| Build Verilog | 1.540 s | 1.399 s |
| First measured full C++ run | 1.015 s | 0.559 s |
| First measured full Verilog run | 0.728 s | 0.681 s |
| Warm C++ median, one worker | 0.122365 s | 0.015493 s |
| Warm Verilog median | 0.014887 s | 0.014436 s |
| Warm native peak RSS median | 2.97 MiB | 2.39 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 19,918,413 bytes | 3,466,217 bytes |
| Generated C++ total | 1,240,866 bytes | 981,451 bytes |
| Generated Verilog total | 276,346 bytes | 209,341 bytes |

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/latency_pipeline --target cpp --cycles 158
pycircuit run examples/latency_pipeline --target verilog --cycles 158
```

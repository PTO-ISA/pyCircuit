# Conditional routing pipeline

`Item` contains a u32 value followed by a u1 route. The D2 source queue routes
each old head into one of two D1 branch-input queues. Route zero selects +10;
route one selects +20. Each transform has its own D1 result queue; a priority
merge feeds a final D1 queue. All six queues have latency one, giving seven
physical slots. Only the selected route destination controls source readiness.

`ConditionalPipeline(valid, data, take)` returns `ConditionalResult`: ready at
bit 34, valid at 33, value at 32:1 and route at 0. Ordinary struct constructors
update the value while retaining route. There are no explicit clock/reset,
proposal wires or new routing primitives in Python.

An unstalled token is captured at E0, routed at E1, transformed at E2, merged
at E3 and first consumed at E4. Work reads old heads, full replacement preserves
occupancy, and empty bypass is disabled. Rising reset empties every queue;
held and falling levels do not transfer. Arithmetic is modulo 2^32.

Unknown value bits produce all-X computed value with exact zero known/Z masks;
latent computed value bits are unspecified. Route is copied unchanged. An
unknown route fails only when an effective transfer remains unknown. When both
destinations are full and blocked, known-zero ready masks all transfers; that
no-transfer edge may successfully commit clock history. Terminal failure tests
must Reset the failed system before reuse.

## Build and verification

```sh
cmake -S examples/conditional_pipeline -B /absolute/build/conditional_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/conditional_pipeline --parallel 4
ctest --test-dir /absolute/build/conditional_pipeline --output-on-failure --no-tests=error
```

Independent identity ledgers must verify selected-only backpressure, exactly-once
retirement, false-branch priority and overtaking, complete queue capacity,
replacement, wrap, reset/hold/discard and masked versus effective unknown route.
Actual generated native workers 1/2 and RTL use the shared installed flow.

The standalone module, system and four-state gates pass 3/3. Native workers 1/2, Verilator and
Icarus agree on 403 known Work samples; native and genuine Icarus agree on
831 four-state samples. Known traffic accepts 140, retires 133 and reset-drops
seven tokens; four-state traffic accepts 337, retires 323 and drops 14. Both
finish empty and reach seven occupied slots. Each schedule checks four priority
grants and six overtaking events. The four-state schedule includes four successful
masked-unknown-route rising edges, followed by a full reset.

Computed value comparisons check known/Z masks and known payload bits; the route
bit is copied exactly. Separate terminal tests cover immediately effective
unknown routing and release after masked success, with mandatory native Reset
recovery and isolated RTL failure processes. The RTL fixture uses explicit
four-state case comparisons for memory-bit knownness: Icarus returned a spurious
unknown result for `$isunknown` on that expression. The original failure and
equivalent predicate repair are retained in the evidence.

## Scenario bench

`bench.py` stores all 184 original scenarios in an immutable typed Table. Its
8-field, 70-bit `Stimulus` preserves field order, exact values and complete
input/expected payloads. Row 184 is all zero, retaining the helper fallback
for every known u16 phase from 184 through 65,535. That fallback is distinct
from the system counter, which holds at 183. The run still lasts 184 cycles.

The explicit Item constructor preserves value32 and route1 independently from
transport valid. Route zero still selects +10, route one +20, with false-branch
merge priority and possible overtaking. The six-queue, seven-slot timing remains
in the unchanged DUT and physical oracles.

The complete advance and system bodies remain unchanged, including ordered
arguments, all unmasked assertions/messages, log order and advance-before-check
registration. Independent source checks compare all 524,288 field values across
the known-u16 domain. Twenty complete native and RTL traces match all 1,840
observations and the terminal result after excluding only changed source-position
metadata, including native workers 1 and 2. Baseline and compact versions each
pass all 3 standalone tests. See the module [excerpts and receipt](GENERATED.md).

The original physical tests retain 403 known Work samples and 831 four-state Work samples.
These are complete run counts, not just minimum assertions. Registered four-state
helpers already execute the existing Icarus paths, including their original
histories and negative probes where present.

Regular-clock systems do not replace physical reset/held-clock histories, and
known-u16 source proof does not establish arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 1,403 lines / 36,121 bytes to
257 lines / 25,835 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 0.993 s | 0.672 s |
| Link system | 1.640 s | 1.249 s |
| Emit C++ | 1.464 s | 1.182 s |
| Emit Verilog | 1.225 s | 0.958 s |
| Build C++ | 1.117 s | 0.920 s |
| Build Verilog | 1.525 s | 1.372 s |
| First measured full C++ run | 0.806 s | 2.770 s |
| First measured full Verilog run | 0.646 s | 0.829 s |
| Warm C++ median, one worker | 0.192273 s | 0.024021 s |
| Warm Verilog median | 0.020970 s | 0.019590 s |
| Warm native peak RSS median | 3.44 MiB | 2.72 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 6,559,667 bytes | 5,199,475 bytes |
| Generated C++ total | 1,730,740 bytes | 1,526,409 bytes |
| Generated Verilog total | 373,711 bytes | 301,628 bytes |

Recorded first-run regressions: native 0.806 → 2.770 s, RTL 0.646 → 0.829 s.
They remain separate from warm medians; one observation does not identify a cause.

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/conditional_pipeline --target cpp --cycles 184
pycircuit run examples/conditional_pipeline --target verilog --cycles 184
```

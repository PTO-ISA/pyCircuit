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

Standalone and aggregate gates each pass 2/2. Native workers 1/2, Verilator and
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
The separate `bench.py` system checks a finite regular-clock known-state scenario with exact old-state output checks. This is partial system migration: the original independent drivers retain their full physical-clock, midstream-reset and four-state scenarios.

```bash
pycircuit run examples/conditional_pipeline --target cpp --cycles 184 --build-dir .pycircuit_out/conditional_pipeline/system-cpp
pycircuit run examples/conditional_pipeline --target verilog --cycles 184 --build-dir .pycircuit_out/conditional_pipeline/system-verilog
```

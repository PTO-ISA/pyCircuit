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
The separate `bench.py` system checks a finite regular-clock known-state scenario, including queue saturation, stalls, replacement, boundary values and final drain. This is partial system migration: the original independent drivers retain their full physical-clock, midstream-reset and four-state scenarios.

```bash
pycircuit run examples/broadcast_pipeline --target cpp --cycles 158 --build-dir .pycircuit_out/broadcast_pipeline/system-cpp
pycircuit run examples/broadcast_pipeline --target verilog --cycles 158 --build-dir .pycircuit_out/broadcast_pipeline/system-verilog
```

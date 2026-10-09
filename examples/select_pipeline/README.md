# Selected-input pipeline

The control stream contains a one-bit route. Two independent u64 lane streams
feed a selected-output queue. Each successful selection consumes the oldest
control and exactly one token from its chosen lane and produces one result.
An available unselected lane cannot bypass an empty selected lane. Unselected
tokens remain queued, preserving each lane's FIFO order and control-head order.
Historical `int` is a 64-bit payload here; the rewrite preserves all64 bits.

The original fixed array of two lanes is written as two explicit queue calls.
This expresses the finite design without claiming general Python array
elaboration. It uses ordinary types, queue connections and conditional
expressions through the existing frontend and common IR.

## Interface and state

`SelectPipeline` takes independent `control_valid`/`control`,
`lane0_valid`/`lane0_data`, `lane1_valid`/`lane1_data`, plus output `take`.
Control is a nominal SelectControl record containing route:u1. SelectResult68
packs control_ready67, lane0_ready66, lane1_ready65, output valid64 and data63:0.

All four owners have depth2 and latency1, for eight complete-token slots and
386 logical payload bits: two control bits plus six 64-bit words. FIFO metadata
and four-state planes are separate representation. No extra stages or slots
are added. An unstalled pair accepted at E0 moves into the result at E1 and first
retires at E2; there is no empty bypass.

Every queue explicitly uses `downstream_pop`. A full result can retire its old
head and accept a selected pair on the same edge; full input queues can refill
only when their corresponding old heads are consumed. Holds and falling clock
levels transfer nothing. Rising reset drops every occupied token and clears all
four queues. Initialized invalid output is packed zero. The compiler generates
hidden physical clock/reset pins driven by the host/SystemRunner; the shared
Runtime performs Work/Xfer and whole-system discard.

## Four-state behavior

Known selectors preserve all chosen data value/known/Z planes, including native
latent values. Unselected X/Z data cannot contaminate the output. For an X/Z
control head with otherwise known controls:

- If the output cannot accept, selection is masked and may stall safely.
- If both lanes are empty, selection is masked; independent input pushes may
  still commit on that edge.
- If the output can accept and either lane has data, an effective transfer is
  unknown and the whole native transaction fails without partial commit.
- Two valid lanes with identical data still fail: equality of payload does not
  establish which token was consumed.

These are existing current FIFO/conditional semantics. Historical native used
an integral selector and old RTL could silently hold unknown transfers; this
example does not claim legacy X/Z parity. A queued X/Z control cannot be fixed
by changing the external route. Terminal executor failure requires Reset.
Setting take=1 reproduces the original native sink's automatic demand;
controlled stalls exercise the historical exposed ready/valid boundary.

## Build and verification

```sh
cmake -S examples/select_pipeline -B /absolute/build/select_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/select_pipeline --parallel 4
ctest --test-dir /absolute/build/select_pipeline --output-on-failure --no-tests=error
```

Independent tests use separate input-acceptance, pair-selection and
retirement ledgers, all64-bit payload patterns and actual generated native/RTL
execution. They must cover head-of-line blocking, skewed sources, unselected
retention, all eight slots, simultaneous retire/select/refill, pointer wrap,
stalls, held changes, busy reset, drain, conservation and every unknown-route
case above. External acceptance and retirement use observed public handshakes;
internal joins are inferred by the independent old-slot model and checked
through output values, latency and capacity, without added debug ports.

Standalone and targeted aggregate gates each pass 2/2. Native workers1/2 and
Verilator agree on 3,659 known Work rows; native and genuine Icarus also agree
on 4,395 four-state rows. There are 218 known u64 patterns through both routes
(436 routed cases), 512 per-bit X/Z cases and16 dense cases with native latent
alternatives. Each case also drains a distinguishable independently admitted
counterpart from the other lane.

Known history accepts 903 controls, 454 lane0 and453 lane1 tokens, retires448/447
by lane and reset-drops8/6/6 by input component. Four-state history accepts
1087/546/545, retires540/539 and drops8/6/6. Both drain empty, reach all eight
slots, perform13 full replacements and exercise two full resets. Result tokens
represent one control and one data component in these conservation accounts.

Two native owner probes verify discard/reprepare and failure without partial
commit. Eight masked/empty unknown-selector cases pass in native and RTL.
Sixteen isolated native terminal cases per worker and eight RTL negative cases
cover effective unknown transfers, including equal lane data. Native failure
requires Reset before recovery. The finite runner bound is6,000 sampling epochs
per successful history.

## Generated system usage

`bench.py` exports `example_select_pipeline.bench.ExerciseSelectPipeline`. Compile
`select_pipeline.py`, then `bench.py`, import the published DUT interface, and link
that explicit system root through the public compile/link/emit flow. Run
`pycircuit run examples/select_pipeline --target cpp --cycles 1829` or select
`--target verilog`. Imported records use their original nominal declarations
and supply every field explicitly.

All original known-stream data edges are represented in this regular-clock
scenario, with fixed independent expectations from the retained native oracle
along a resetless trajectory. The 1829 cycles include a final observation.
The original DUT, native/RTL drivers, finite configuration, and any four-state,
reset/discard, latency and token-ledger matrices remain unchanged. Physical
held-level and midstream-reset scenarios still require those original module
oracles; this system does not claim complete physical-scenario equivalence.

The complete 1,829-cycle system passes native workers 1 and 2 and Verilator.
Each run produces 3,658 evaluation epochs and 18,290 scalar observations; all
five public values at every epoch match the retained independent native oracle.
All five source assertions remain active. The native runs use the same compiled
artifact, and their execution inputs, source files and compiler helpers were
verified unchanged across the runs. The final receipts and complete compressed
traces are in `docs/gates/logs/20261008-pr271-select-full`.

The source-check dependency analysis issue is resolved without reducing the
fixture or increasing its work budget. The generated native fixture header is
14 MB; GCC `-O3` compilation exceeded the build timeout, and a Clang `-O1`
attempt was cancelled during compilation. Clang 22 with `-O0` completed the
native build and both full runs. These build choices leave the stimulus,
expected values, cycle count and original module oracles unchanged.

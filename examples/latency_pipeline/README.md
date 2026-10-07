# Delayed increment pipeline

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
the source tree. The same two gates also run in the aggregate examples build.

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
# Record projection update

Force the payload valid bit to one while preserving Header, tag and payload.

## Payload and interface

`RecordProjectionUpdate(valid, data, take)` returns a typed Result with `ready`, `valid`
and a 29-bit data value. **Payload valid and transport valid are independent.**
An input packet whose payload valid is zero is still a token and must transfer.

The update uses ordinary record copy and field assignment. It sets bit0 to a
known one and preserves all other 28 bits, including X/Z in Header, tag and
payload. No field-by-field reconstruction of the unchanged packet is needed.

## Original timing and current source

The historical typed input and rule result each owned a depth1/latency1 queue.
Returning the result added a sink boundary, not a third queue. The current
module retains exactly two one-token queues with explicit `downstream_pop`
ready policy, matching the old RTL full-replacement behavior. The pure record
operation between them adds no storage.

A token accepted at E0 enters the result queue at E1 and can be consumed at E2.
No empty flow-through is allowed. Stalls fill at most two slots; full queues
can simultaneously consume, advance and accept on one rising edge. Held clocks
do not transfer. Reset empties both stages; Work samples old committed state.
The driver owns physical clock/reset and output collection.

## Build and verification

```sh
cmake -S examples/record_projection_update -B /absolute/build/record_projection_update -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/record_projection_update --parallel 4
ctest --test-dir /absolute/build/record_projection_update --output-on-failure --no-tests=error
```

The two CTest gates execute the actual generated DUT with native workers 1/2,
Verilator, and genuine Icarus X/Z simulation. Each checks 101 old-state Work
samples, all 29 input bit positions, startup, full replacement, stalls,
held-high/held-low clocks, drain, and reset. Payload-valid zero tokens transfer
normally. Four native cases per worker and four Icarus cases check selected
X/Z bits and masks.

A separate edge-qualified history ledger records 40 accepted tokens, 38 retired,
2 discarded by reset, zero outstanding, and peak occupancy 2. Only rising edges
count transfers; repeated Work samples do not duplicate tokens. Main runner
and four-state helper runs both have finite limits.
The separate `bench.py` system checks a finite regular-clock known-state scenario with exact old-state output checks. This is partial system migration: the original independent drivers retain their full physical-clock, midstream-reset and four-state scenarios.

```bash
pycircuit run examples/record_projection_update --target cpp --cycles 184 --build-dir .pycircuit_out/record_projection_update/system-cpp
pycircuit run examples/record_projection_update --target verilog --cycles 184 --build-dir .pycircuit_out/record_projection_update/system-verilog
```

# Named record projection

Project the payload valid bit and nominal Header into HeaderView.

## Payload and interface

`RecordProjection(valid, data, take)` returns a typed Result with `ready`, `valid`
and a 5-bit data value. **Payload valid and transport valid are independent.**
An input packet whose payload valid is zero is still a token and must transfer.

HeaderView stores payload valid first, then Header: valid occupies bit4 and
opcode occupies bits3:0. This explicitly moves the original Packet valid bit0
and header bits28:25. Tag and payload are omitted; it is not a low-bit slice.
Projection preserves X/Z in the selected fields.

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
cmake -S examples/record_projection -B /absolute/build/record_projection -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/record_projection --parallel 4
ctest --test-dir /absolute/build/record_projection --output-on-failure --no-tests=error
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
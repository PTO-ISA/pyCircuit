# Record composition with atomic stream joins

## Payloads and interface

Each of base, header, payload and patch has a separate input valid and returned
ready. `take` controls output consumption. Result contains the four readies in
that order, output valid, and Packet data (30 bits total). Packet.valid and
Patch.valid are payload bits: a zero does not suppress a transferred token.

The final packet contains Header.opcode, Patch.tag, Payload.data and Patch.valid.
Header.tag is replaced by Patch.tag. The base packet's bits do not affect the
result; omitting its token prevents composition.

## Preserved hardware timing

The first join consumes base, header and payload together when all are available
and the composition queue is ready. Each input's out-ready is composition-ready
AND the other two inputs' valid signals. The second join consumes composition
and patch together when both are available and the output queue is ready.
A waiting input cannot be consumed alone, even if its value is unused.

With all inputs arriving together, E0 captures inputs, E1 commits composition,
E2 commits the update, and E3 is the first possible output consumption. Different
input arrival times and downstream stalls extend this latency. All decisions use
old committed state; queues support simultaneous pop/push but no empty bypass.
Held clocks do not transfer. Reset clears all six queues on a rising edge.
Python contains no physical clock/reset or proposal wires; the host driver owns
those pins through the existing typed DUT and SystemRunner interface.

## Build and verification

```sh
cmake -S examples/record_spread_pipeline -B /absolute/build/record_spread_pipeline -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/record_spread_pipeline --parallel 4
ctest --test-dir /absolute/build/record_spread_pipeline --output-on-failure --no-tests=error
```

The two CTest gates run the generated native DUT with workers 1/2, Verilator,
and genuine Icarus. Each compares 317 old-state Work samples, including separate
missing-base/header/payload/patch sequences, full six-slot replacement,
changing rejected inputs, repeated high/low clocks, reset, and drain. Thirty
directed records exercise all 54 physical input bit positions. Four cases per
native worker and four Icarus cases verify X/Z preservation and overridden fields.

The independent ledger records 158 accepted input components, 36 output packets,
14 components discarded by reset, zero outstanding, and peak occupancy six.
Composition fires 39 times, patch application 37 times; each consumes its whole
input set. The four input acceptance counts are 40/40/40/38. Because a composed
token contains three inputs and an output token contains four, conservation is
`accepted = 4 * retired + reset_dropped + outstanding_components`, rather than
counting the six differently placed tokens as interchangeable packet capacity.
Main and auxiliary runners have finite limits.
# DMA between two memories

This example copies a complete request through two independent memories. The
DRAM response carries the source data and both addresses to the SRAM writer.
The copy acknowledgement returns the old destination data. A separate check
request reads the new destination value.

The source uses ordinary typed modules and one shared state-update rule. Queue
capacities, memory delays and read-first priorities are design policy expressed
in Python; the framework uses its existing compile/link/emit and Runtime flow.
The local executable is a consumer example, outside framework SDK exports.

## Requests and responses

`DmaRequest` contains `dram_address:u4`, `sram_address:u4`, `data:u16` and `tag:u8`.
Every response retains both addresses and the tag; only `data` is replaced with
the old RAM value. Root inputs have independent valid/request pairs for seed,
copy and check, plus one response-consumption signal per channel. `DmaResult`
returns each input's capacity-ready signal and each output's valid/full packet.
Invalid output payloads are unspecified.

The result also exposes pure two-bit accepted/enqueued observation masks for each
RAM. Bit 0 denotes the higher-priority reader: DRAM copy or SRAM check; bit 1
is the writer. These observations allocate no state and do not affect scheduling.

## Storage and timing

| Owner | Requests | Responses | Delay and priority |
| --- | --- | --- | --- |
| DRAM, 16×u16 | copy queue depth 4; seed queue depth 2 | copy-data and seed-ack queues, depth 2 each | 3 rising edges; copy read before seed write |
| SRAM, 16×u16 | DRAM copy-data queue; check queue depth 2 | copy-ack and check-data queues, depth 2 each | 2 rising edges; check read before copy write |

All seven queues have latency 1 and total capacity 16 packets. Each memory can
also hold one outstanding transaction. The DRAM copy-data queue connects directly
to SRAM; there is no extra connection queue. Each RAM and its saved transaction
state belongs to a separate module instance.

Acceptance writes RAM and saves the request/endpoint. The controller captures old
RAM data on the following rising edge, preserving it across stalls beyond the
primitive's read-output lifetime. An eligible response waits for its own response
queue to have space. The release edge cannot also accept a new request. Full
queues use the standard local-occupancy policy, without implicit replacement.
Fixed priority can starve writers under an infinite read stream.

## Original contract and validation

The independent two-RAM/seven-queue oracle passes 1,167 Work samples and 581
rising edges with native workers 1/2 and Verilator. It checks all flags and valid
packet bits, all seven capacities, both priorities, long stalls, concurrent
service and reset retention. Of 83 accepted inputs, 70 retire and 13 are canceled
by reset. The native and RTL tests cover this behavior directly.

Native probes discard a fully prepared graph and reject an unknown seed-valid
guard before RAM proposals. Two completed copy responses detect early RAM write
leaks via old-data checks; a third response is canceled by reset. Late failures
after both RAMs prepare, Z-control cases and RTL fault rollback are not covered
by this example. No cross-platform acceptance is implied by a local host run.

## Build and run

```sh
cmake -S examples/dma -B /absolute/build/dma -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/dma --parallel 4
ctest --test-dir /absolute/build/dma --output-on-failure --no-tests=error
```

The shared example verifier runs the generated native DUT with workers 1 and 2,
then compares equivalent settled precommit RTL observations. Testbench inputs and
oracles are separate from the Python design.

## Generated system usage

`bench.py` exports `example_dma.bench.ExerciseDma`. Compile
`dma.py`, then `bench.py`, import the published DUT interface, and link
that explicit system root through the public compile/link/emit flow. Run
`pycircuit run examples/dma --target cpp --cycles 582` or select
`--target verilog`. Imported records use their original nominal declarations
and supply every field explicitly.

All original known-stream data edges are represented in this regular-clock
scenario, with fixed independent expectations from the retained native oracle
along a resetless trajectory. The 582 cycles include a final observation.
The original DUT, native/RTL drivers, finite configuration, and any four-state,
reset/discard, latency and token-ledger matrices remain unchanged. Physical
held-level and midstream-reset scenarios still require those original module
oracles; this system does not claim complete physical-scenario equivalence.

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

## Scenario bench

`bench.py` stores 294 immutable scenario intervals. Each row keeps the original
40 fields (212 bits) and an inclusive 64-bit `end`. The existing Table query
`rows.first(where=lambda row: phase <= row.end)` selects the first matching row.
Ends increase strictly, with a final endpoint of 18,446,744,073,709,551,615, so
every known u64 phase has a match. The existing counter wraps modulo 2⁶⁴.
Intervals avoid duplicating long stretches of identical scenario data.

The final interval starts at 581. Its three response-consumption fields,
`expected_copy_ready`, `expected_check_ready` and `expected_dram_enqueued` are
one; every other scenario field is zero, including `expected_seed_ready`.
That tail preserves helper values; it does not assert DUT steady-state behavior
beyond the original assertion window.

All three DmaRequest constructors retain their four explicit fields. The 22
assertions remain guarded by `phase < 582`. Each of the twelve response-field
comparisons retains its original expected-valid mask and bitwise OR, so invalid
payloads remain unconstrained. All 22 observations remain unconditional, and
`check()` still precedes `advance(phase)`.

The original and compact sources agree on all 23,280 active field values and
the complete known-u64 phase domain, including the tail and wrap transition.
This is a source-level proof, not a simulation through 2⁶⁴ cycles. Twenty
complete 582-cycle C++/Verilog traces match after removing only changed
source-position metadata, including native workers 1 and 2. Each has 25,608
observations and its terminal result. The standalone module and system gates
pass 2/2 for both versions. See the module [excerpts and receipt](GENERATED.md).

The closed system follows a resetless regular-clock trajectory. Original module
drivers still own the 1,167-sample two-RAM/seven-queue ledger, reset/discard and
failure probes described above. The compact helper does not claim arbitrary
X/Z-phase equivalence or complete physical-scenario coverage.

## Local cost comparison

The source shrinks from 3,711 lines / 138,699 bytes to
485 lines / 87,577 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, emit, build
and first-run times are single serial observations; warm medians use three
alternating full-length pairs. Every fresh process includes initialization.
These measurements do not guarantee results on other platforms or optimization
levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 5.152 s | 4.056 s |
| Emit C++ | 9.353 s | 9.509 s |
| Emit Verilog | 7.967 s | 5.445 s |
| Build C++ | 3.572 s | 2.855 s |
| Build Verilog | 3.085 s | 2.651 s |
| First measured full C++ run | 4.560 s | 1.321 s |
| First measured full Verilog run | 0.879 s | 0.982 s |
| Warm C++ median, one worker | 2.380 s | 0.685 s |
| Warm Verilog median | 0.212 s | 0.221 s |
| Warm native peak RSS median | 7.52 MiB | 7.81 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 27,298,328 bytes | 30,184,988 bytes |
| Generated C++ total | 5,878,508 bytes | 9,917,303 bytes |
| Generated Verilog total | 1,199,974 bytes | 1,641,335 bytes |

This representation improves source size and native execution but has measurable
costs: warm Verilog is about 4% slower, C++ emission rises from 9.353 to 9.509 s,
and native peak RSS and both generated-code totals increase. The first Verilog
run also takes longer. Interval lookup retains query work; immutable constant
materialization does not remove that cost. This is not an across-the-board
performance improvement.

Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/dma --target cpp --cycles 582
pycircuit run examples/dma --target verilog --cycles 582
```

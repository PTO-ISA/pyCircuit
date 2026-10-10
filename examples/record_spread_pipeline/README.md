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

The module and four-state CTest gates run the generated native DUT with workers 1/2, Verilator,
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

## Scenario bench

`bench.py` stores 158 regular-clock scenarios in an immutable typed Table. Its
23 fields retain the original 89-bit stimulus and expectation tuple. The
64-bit phase selects its own row below 158 and row 157 thereafter; rows 155–157
are deliberately repeated. From phase 155 through the largest known u64 value,
only `take` and the four expected-ready fields are one. The existing counter
increment wraps modulo 2⁶⁴.

Protocol `base_valid` and `patch_valid` remain separate from the payload bits
`base_present` and `patch_present`. Every Packet/Header/Payload/Patch constructor
retains its explicit fields, including base payload that composition overwrites.
All nine assertions remain unmasked under `phase < 158`; all nine observations
remain unconditional, and `check()` still precedes `advance(phase)`.

The original and compact sources agree on all 3,634 active field values and the
complete known-u64 phase domain, including the tail and wrap transition. This is
a source-level proof, not a simulation through 2⁶⁴ cycles. Twenty complete
158-cycle C++/Verilog traces match after removing only changed source-position
metadata, including native workers 1 and 2. Each has 2,844 observations and its
terminal result. The module, system and four-state gates pass 3/3 for both
versions. See the module [excerpts and receipt](GENERATED.md).

The closed system follows a resetless regular-clock trajectory. Original module
drivers still own the 317-sample physical clock/reset, six-slot conservation and
X/Z oracles described above. The compact helper does not claim arbitrary
X/Z-phase equivalence or complete physical-scenario coverage.

## Local cost comparison

The source shrinks from 3,583 lines / 127,524 bytes to
280 lines / 32,840 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, emit, build
and first-run times are single serial observations; warm medians use three
alternating full-length pairs. Every fresh process includes initialization.
These measurements do not guarantee results on other platforms or optimization
levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 3.214 s | 1.216 s |
| Emit C++ | 4.263 s | 2.541 s |
| Emit Verilog | 2.991 s | 1.741 s |
| Build C++ | 1.749 s | 1.354 s |
| Build Verilog | 1.653 s | 1.575 s |
| First measured full C++ run | 0.960 s | 0.604 s |
| First measured full Verilog run | 0.483 s | 0.329 s |
| Warm C++ median, one worker | 0.404 s | 0.034 s |
| Warm Verilog median | 0.029 s | 0.028 s |
| Warm native peak RSS median | 5.39 MiB | 3.61 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 17,846,352 bytes | 9,570,280 bytes |
| Generated C++ total | 3,495,631 bytes | 2,977,713 bytes |
| Generated Verilog total | 748,639 bytes | 517,905 bytes |

Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/record_spread_pipeline --target cpp --cycles 158
pycircuit run examples/record_spread_pipeline --target verilog --cycles 158
```

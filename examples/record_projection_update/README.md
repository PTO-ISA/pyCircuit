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

The module and four-state CTest gates execute the actual generated DUT with native workers 1/2,
Verilator, and genuine Icarus X/Z simulation. Each checks 101 old-state Work
samples, all 29 input bit positions, startup, full replacement, stalls,
held-high/held-low clocks, drain, and reset. Payload-valid zero tokens transfer
normally. Four native cases per worker and four Icarus cases check selected
X/Z bits and masks.

A separate edge-qualified history ledger records 40 accepted tokens, 38 retired,
2 discarded by reset, zero outstanding, and peak occupancy 2. Only rising edges
count transfers; repeated Work samples do not duplicate tokens. Main runner
and four-state helper runs both have finite limits.

## Scenario bench

`bench.py` stores all 184 original scenarios in an immutable typed Table. Its
12-field, 62-bit `Stimulus` retains the original schema and field order. Row 184
is all zero, preserving the helper fallback for every known u16 phase from 184
through 65,535. This fallback is distinct from the system counter, which holds
at phase 183. The run retains its original 184-cycle limit.

Protocol valid and payload valid remain separate fields. All 76 protocol-valid
rows with payload-valid zero are retained, along with the explicit nested
Header/Packet constructors. Both observations named `valid` remain in order:
protocol `dut.valid` first, payload `dut.data.valid` last. They are not renamed,
merged or deduplicated during trace comparison.

The complete system retains all 6 unconditional assertions, 7 observations,
ordered DUT inputs and `advance(phase)` before `check()`. Independent source
checks compare all 786,432 field values across the complete known-u16 helper
domain. Twenty complete native and RTL traces match all 2,576 observations
and the terminal result after excluding only changed source-position metadata;
native workers 1 and 2 are included. Baseline and compact versions each pass
all three standalone gates. See the module [excerpts and receipt](GENERATED.md).

The closed system covers a regular-clock known-state trajectory. Original
physical reset, held-clock and four-state histories remain with the unchanged
independent module drivers described above. Known-u16 source equivalence does
not establish arbitrary X/Z-phase equivalence or full physical-scenario migration.

## Local cost comparison

The source shrinks from 2,142 lines / 59,528 bytes to
277 lines / 42,499 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 1.561 s | 1.059 s |
| Link system | 2.353 s | 1.879 s |
| Emit C++ | 2.098 s | 1.790 s |
| Emit Verilog | 1.713 s | 1.413 s |
| Build C++ | 1.327 s | 1.131 s |
| Build Verilog | 1.533 s | 1.436 s |
| First measured full C++ run | 0.600 s | 0.318 s |
| First measured full Verilog run | 0.333 s | 0.310 s |
| Warm C++ median, one worker | 0.289974 s | 0.028803 s |
| Warm Verilog median | 0.027099 s | 0.026118 s |
| Warm native peak RSS median | 4.33 MiB | 3.09 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 10,298,360 bytes | 8,013,950 bytes |
| Generated C++ total | 2,552,729 bytes | 2,200,093 bytes |
| Generated Verilog total | 565,081 bytes | 430,279 bytes |

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/record_projection_update --target cpp --cycles 184
pycircuit run examples/record_projection_update --target verilog --cycles 184
```

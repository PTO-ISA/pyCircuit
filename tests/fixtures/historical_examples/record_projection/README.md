# Named record projection

The issue #319 source migration is an unaccepted savepoint pending generated
C++/Verilog execution of compiler-owned queue admission. Named ingress and
egress handles retain the two depth-one, latency-one `downstream_pop` queues.
One registered `transfer` Rule pops the old ingress head, computes the payload
and pushes the egress atomically; the compiler supplies availability and capacity
guards. Raw `in_valid`/`in_data` and `out_ready` bindings retain the external
handshake, and `ready`/`valid`/`data` expose the queue boundary views. Test counts,
measurements and generated excerpts below describe the preceding implementation
and are not acceptance evidence for this migration.

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
10-field, 38-bit `Stimulus` preserves the original schema, field order and all
input/expected payloads. Row 184 is all zero, retaining the helper fallback
for every known u16 phase from 184 through 65,535. This fallback is distinct
from the system counter, which holds at 183. The run retains 184 cycles.

The complete Packet/Header constructor retains tag and payload even though the
DUT projects them away. Protocol valid and payload valid remain distinct,
including 76 protocol-valid rows with payload-valid zero. Both adjacent log
events named `valid` remain in order: `dut.valid`, then `dut.data.valid`.
Trace comparison preserves their multiplicity and never deduplicates by label.

The complete advance and system bodies are unchanged, including unconditional
assertions, observation order and advance-before-check/exercise registration.
Independent source checks compare all 655,360 field values across the known-u16
helper domain. Twenty complete native and RTL traces match all 1,840 observations
and the terminal result after excluding only changed source-position metadata,
including native workers 1 and 2. Baseline and compact versions each pass all
three standalone tests. See the module [excerpts and receipt](GENERATED.md).

Original physical tests retain 101 known Work samples and four full-DUT X/Z cases,
with the reset, held-clock and conservation checks described above. The
registered four-state test already executes Icarus. Regular-clock systems do not
replace physical histories, and known-u16 source proof does not establish
arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 1,731 lines / 45,437 bytes to
266 lines / 32,145 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 1.251 s | 0.899 s |
| Link system | 1.910 s | 1.605 s |
| Emit C++ | 1.694 s | 1.516 s |
| Emit Verilog | 1.432 s | 1.201 s |
| Build C++ | 1.256 s | 1.064 s |
| Build Verilog | 1.568 s | 1.497 s |
| First measured full C++ run | 0.633 s | 0.128 s |
| First measured full Verilog run | 0.251 s | 0.282 s |
| Warm C++ median, one worker | 0.236047 s | 0.021769 s |
| Warm Verilog median | 0.021304 s | 0.020141 s |
| Warm native peak RSS median | 3.81 MiB | 2.86 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 8,021,559 bytes | 6,397,250 bytes |
| Generated C++ total | 2,042,867 bytes | 1,799,172 bytes |
| Generated Verilog total | 450,845 bytes | 353,906 bytes |

The first measured RTL run increases from 0.251 to 0.282 s despite the lower
warm median. That regression is retained separately; one observation does not
identify its cause or establish steady-state behavior.

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/record_projection --target cpp --cycles 184
pycircuit run examples/record_projection --target verilog --cycles 184
```

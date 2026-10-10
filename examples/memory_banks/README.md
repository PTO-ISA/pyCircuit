# Four independent memory banks

One reusable `Bank` module is called four times. Requests enter a shared queue,
route to the bank named in its oldest packet, and return through a fixed-priority
merge. A busy selected bank blocks the root head; later requests do not bypass it.
Different banks can serve transactions concurrently.

The source uses existing module/rule/queue/sync_mem constructs. Bank selection,
capacity, latency and merge priority are source policy. Its ordinary consumer
executable uses the existing public compiler and Runtime; it adds no framework
specialization, executable, IR interface or alternative simulation path.

## Interface

`BankRequest` contains `bank:u2`, `offset:u4`, `write:u1`, `data:u16`, `tag:u8`.
Every response retains all request fields, replacing only `data` with the old
memory word. A write updates the selected bank at acceptance and still returns
the previous value; a read leaves memory unchanged.

`MemoryBanks(valid, request, take)` returns a named `BanksResult` with input ready,
output valid and the full response. Invalid response payload is unspecified.
Pure four-bit observations expose route, acceptance, response enqueue, available
bank responses and selected merge; bit `i` denotes bank `i`. They add no state.

## Storage and timing

The root owns an eight-packet request queue and a two-packet merge queue. Each of
four separate Bank instances owns a two-packet request queue, one 16×u16 RAM,
a 35-bit controller state and a two-packet response queue. There are ten queues
with 26 total slots, plus at most four controller-held transactions. All queues
have latency1 and local-occupancy ready without implicit full replacement.

Each bank enqueues its response no earlier than two rising edges after acceptance.
It saves old RAM data on the first following edge so backpressure cannot outlive
the primitive's read-output lifetime. A bank cannot accept on its release edge.
The merge selects bank0 before bank1, bank2 and bank3; only the selected response
is popped when the merge queue has room. Per-bank order is preserved, while
responses from different banks can reorder. Fixed priority is not a fairness
promise under an infinite high-priority stream.

## Original contract and verification

Prior native and RTL verification covers:
883 Work samples and 439 rising edges agree on native workers1/2 and Verilator.
The independent oracle uses four RAM arrays, ten deques and absolute deadlines.
It checks all 22 flags, all 31 valid packet bits, unique tags, per-bank order, every
queue capacity, parallel service, head-of-line blocking and fixed-priority merge.
Of 136 accepted inputs, 102 retire and 34 are canceled by reset.

Three native generated-root probes prepare/discard/retry merge edges with
non-idempotent writes; subsequent old-data responses detect leaked writes.
Selected unknown output-pop guards also reject and retry. These direct
Work/Discard/Xfer probes do not establish automatic SimSystem failure handling,
arbitrary fault sites, native Z-control or RTL fault rollback.

## Build and run

```sh
cmake -S examples/memory_banks -B /absolute/build/memory_banks -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/memory_banks --parallel 4
ctest --test-dir /absolute/build/memory_banks --output-on-failure --no-tests=error
```

The existing shared verifier runs native workers1/2 and RTL from one final IR,
comparing settled precommit Work observations. Finite SystemRunner configuration
and testbench time bounds prevent an unbounded run. Local verification is not a
claim of full-nightly or cross-platform acceptance.

## Generated system usage

`bench.py` exports `example_memory_banks.bench.ExerciseMemoryBanks`. Compile
`memory_banks.py`, then `bench.py`, import the published DUT interface, and link
that explicit system root through the public compile/link/emit flow. Run
`pycircuit run examples/memory_banks --target cpp --cycles 440` or select
`--target verilog`. Imported records use their original nominal declarations
and supply every field explicitly.

The bench stores 440 immutable `Scenario` rows in one typed Table. Its 19
ordered fields retain 86 bits in total, and zero-valued fields use existing
implicit-zero Struct initialization. The author extracted the literal records
from the frozen original AST without importing or executing design code; this
is separate from independent preservation of all 8,360 active values.

The full `bits[64]` phase starts at zero, selects its corresponding row below
440 and row 439 thereafter, and increments with u64 wrap. Direct projections
supply all five explicit `BankRequest` fields and the original three DUT
arguments. Twelve assertions retain their `phase < 440` guard, empty else,
messages and order. Each response comparison retains the exact bitwise guard
`(scenario.expected_output_valid == 0) | (dut.response.FIELD ==
scenario.expected_response_FIELD)`. It uses expected output validity, distinct
from the response-valid bank mask. Twelve logs remain unconditional, and
registration still calls `check()` then `advance(phase)`.

From phase 439 through 2**64-1, inputs keep take asserted and every other input
zero. The frozen expected row retains input-ready and output-valid one, response
(bank=1, offset=5, write=1, data=36889, tag=179), route=0, accepted=8, enqueued=0,
response-valid=12 and merged=4. These are preserved source expectations, not a
prediction that the physical DUT indefinitely holds those outputs. Assertions
end at phase 440, while logs continue. Phase wrap restarts row zero without a
new DUT reset.

All original known-stream data edges and fixed expectations remain on the
resetless trajectory; the complete 440-cycle run includes its final observation.
The original DUT, native/RTL drivers, finite configuration, and any four-state,
reset/discard, latency and token-ledger matrices remain unchanged. Physical
held-level and midstream-reset scenarios still require those original module
oracles; this system does not claim complete physical-scenario equivalence.

## Verification and measured costs

The current standalone module and system tests pass 2/2. The module retains its
883-sample independent oracle and discard/retry probes. The system completes 440 cycles
/ 880 epochs, twelve source-check definitions and 10,560 observations across native
workers 1/2 and Verilator.

The independent source proof preserves all 8,360 field values and 223 intervals over the
complete known-u64 domain, including the frozen terminal expectations and modulo wrap.
Fourteen mutated candidates are rejected. Complete normalized AST comparison preserves
every assertion message/operator/target, all five bitwise expected-valid masks, every
log and registration.

Complete original and candidate observations agree across native workers 1/2 and RTL.
The physical-driver probes remain separate from the known-phase source proof; this
rewrite establishes no additional whole-DUT four-state coverage. Neither a finite run nor this source proof claims simulated u64 rollover
or arbitrary injected X/Z phase equivalence. See the current [verified module excerpts
and receipt](GENERATED.md).

The bench shrinks from 4,848 lines / 196,753 bytes to 545 lines / 103,537 bytes.
Measurements use the same machine, installed compiler and LLVM 22 C++ toolchain with
`-O0`. Build/emission phases are single observations from sequential baseline/candidate
pipelines. Runtime medians use three additional paired warm runs in alternating order
and the complete registered cycle count. These limited observations are not cross-platform guarantees.

| Phase | Original | Scenario table |
| --- | ---: | ---: |
| Compile bench | 6.70 s | 3.46 s |
| Link system | 7.64 s | 5.99 s |
| Emit C++ | 6.81 s | 6.30 s |
| Build C++ simulator | 3.10 s | 3.17 s |
| Emit Verilog | 5.69 s | 4.37 s |
| Build Verilog simulator | 2.27 s | 1.92 s |
| Run C++, one worker (warm median) | 2.069 s | 2.199 s |
| Run Verilog (warm median) | 0.102 s | 0.101 s |

Native runtime median increases by 6.3% (about 0.130 seconds). Generated C++ grows 21.8%
and generated RTL 3.8%. These are authoring/frontend improvements with backend costs,
not an overall performance improvement.

Final IR decreases from 31,060,744 to 24,558,398 bytes. Generated C++ files total
6,730,840 → 8,196,204 bytes; generated Verilog files total 1,303,952 → 1,353,345 bytes.

The generated C++ currently rebuilds constant table field planes during Work. One-time
materialization of proven constant planes is a separate generic emitter optimization
tracked in [issue #265](https://github.com/PTO-ISA/pyCircuit/issues/265); no such
compiler change is part of this example rewrite.

Complete source-import and transformed artifacts are retained locally and reproduce both
published units byte-for-byte. Public execution and measured builds share identical
final IR and generated outputs. DUTs, drivers, configurations, cycle limits and timeouts
remain unchanged.

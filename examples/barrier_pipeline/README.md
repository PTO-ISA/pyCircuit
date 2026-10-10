# Heterogeneous barrier pipeline

Independent `LeftToken(value: u16)` and `RightToken(value: u32)` streams each
have a depth-two input and depth-two output queue. All four queues have latency
one and explicit `downstream_pop` readiness. Both old input heads move together
only when both outputs have room. Output consumers retire independently. The
two nominal payload types, positional pairing, eight physical contributor slots
and four slots per stream are preserved.

The implementation uses four ordinary `ac.queue` allocations and cross-connected
ready/valid expressions. Existing field-sensitive dependency analysis verifies
the completed connections. There is no extra payload register or barrier
primitive. Both backends consume the same verified common IR.

`BarrierPipeline(left_valid, left_data, left_take, right_valid, right_data,
right_take)` returns `BarrierResult`: left_ready at bit 51, left_valid at 50,
left_data at 49:34, right_ready at 33, right_valid at 32 and right_data at 31:0.
Each token retains its named `value` field. Python clock/reset and storage
proposal details stay hidden.

Work observes old heads; Xfer commits an accepted edge after whole-system
checking. Full replacement retires the old head and retains occupancy. There
is no empty bypass. Rising reset empties all four queues; held and falling
levels do not transfer. Initialized empty data is zero. Payload transport
preserves the complete value/known/Z planes, without arithmetic conversion.

## Build and verification

```sh
cmake -S examples/barrier_pipeline -B /absolute/build/barrier_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/barrier_pipeline --parallel 4
ctest --test-dir /absolute/build/barrier_pipeline --output-on-failure --no-tests=error
```

The shared installed helper independently compiles this source, links the explicit
closure and emits both targets. Independent tests must cover unequal input
arrivals, one blocked output preventing both moves, independent output
retirement, per-stream token conservation, capacity, wrap, raw four-state
transport, reset, held clocks and whole-system failure/discard. Native workers
1 and 2, Verilator and genuine Icarus use the actual generated DUT.

Prior standalone generated-DUT gates passed: native workers 1/2, Verilator and Icarus
agree on 699 known Work samples; native and genuine Icarus agree on another
619 four-state samples. Native checks all copied value/known/Z planes, including
latent values. Known histories accept 313, retire 305 and reset-drop eight tokens
per stream; four-state histories accept 273, retire 265 and drop eight per
stream. Both finish empty and reach the full eight-slot capacity.

The paired ledger records 309 known and 269 four-state atomic movements.
Each schedule requires independently retired outputs in both directions,
blocked-output stalls in both directions, full replacements and two full resets.
Public-owner probes exercise explicit and late-sibling discard/reprepare.
Separate failed-system tests check terminal failure and mandatory Reset recovery;
an isolated RTL process checks the effective-unknown-transfer diagnostic.

## Generated system usage

`bench.py` exports `example_barrier_pipeline.bench.ExerciseBarrierPipeline`. Compile
`barrier_pipeline.py`, then `bench.py`, import the published DUT interface, and link
that explicit system root through the public compile/link/emit flow. Run
`pycircuit run examples/barrier_pipeline --target cpp --cycles 347` or select
`--target verilog`. Imported records use their original nominal declarations
and supply every field explicitly.

The bench stores 347 immutable `Scenario` rows in one same-source Table. Its
12 original fields retain their declared widths, totaling 104 bits. Zero-valued
fields use existing implicit-zero Struct initialization. The author extracted
the literal rows from the frozen original AST without executing or importing
design code; that extraction is separate from independent preservation.

The full `bits[64]` phase starts at zero and selects its corresponding row below
347, then row 346. Both final rows are retained. From phase 345 through
2**64-1, only the two take and two expected-ready fields are one; all other
fields remain zero. The original phase increment wraps to zero without a new
DUT reset. Selection does not narrow or saturate phase.

Direct projections supply both imported token constructors and the six ordered
DUT arguments. Six assertions keep their `phase < 347` guard, empty else and
messages, while six logs remain unconditional. Registration still calls `check()`
then `advance(phase)`. All original known-stream data edges and fixed expectations
remain on the resetless trajectory; the complete 347-cycle run includes its
final observation.
The original DUT, native/RTL drivers, finite configuration, and any four-state,
reset/discard, latency and token-ledger matrices remain unchanged. Physical
held-level and midstream-reset scenarios still require those original module
oracles; this system does not claim complete physical-scenario equivalence.

## Verification and measured costs

The current module, system and four-state tests pass 3/3. Native workers 1/2, Verilator and Icarus retain 699 known Work samples; native and Icarus retain 619 four-state samples. The system completes 347 cycles / 694 epochs, six source-check definitions and 4,164 observations.

An independent source check preserves all 4,164 active field values, all 340 intervals over the complete known-u64 domain, the constant tail and modulo wrap. Twelve mutated candidates are rejected. A finite run does not execute 64-bit rollover.

Complete original and candidate observations agree across native workers 1/2 and RTL. The physical-control, reset/discard, token-ledger and four-state oracles remain separate from this known-phase source proof; arbitrary injected X/Z phase equivalence is not claimed. See the current [verified module excerpts and receipt](GENERATED.md).

The bench shrinks from 5,318 lines / 242,153 bytes to 435 lines / 95,080 bytes. Measurements use the same machine, installed compiler and LLVM 22 C++ toolchain with `-O0`. Build/emission entries are single observations from serial baseline and candidate pipelines; runtime entries are medians of three additional paired warm runs in alternating order, using the complete registered cycle count. These are example-specific observations, not a cross-platform guarantee.

| Phase | Original | Scenario table |
| --- | ---: | ---: |
| Compile bench | 5.37 s | 2.21 s |
| Link system | 6.19 s | 3.43 s |
| Emit C++ | 5.45 s | 3.22 s |
| Build C++ simulator | 2.15 s | 1.87 s |
| Emit Verilog | 4.26 s | 2.50 s |
| Build Verilog simulator | 1.76 s | 1.82 s |
| Run C++, one worker (warm median) | 2.362 s | 1.853 s |
| Run Verilog (warm median) | 0.080 s | 0.080 s |

The observed native warm median decreases by about 22%; RTL runtime is approximately unchanged. First-run timing variation is retained in the raw evidence; runtime comparisons use the paired warm samples.

Final IR falls from 24,824,279 to 14,925,041 bytes. Generated C++ files total 4,535,036 → 4,229,225 bytes; generated Verilog files total 986,863 → 815,199 bytes.

Complete source-import and transformed artifacts are retained locally and reproduce both published source units byte-for-byte. Public execution and measured builds share identical final IR and generated outputs. No compiler API, DUT storage, scenario count, cycle limit or timeout changes are required.

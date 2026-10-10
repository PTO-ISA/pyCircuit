# Feedback pipeline

`FeedbackPipeline(valid, data, take)` returns a 38-bit result: ready, valid and a 36-bit
Item. Three queues have depths 2/1/1 and latency 1: source, internal feedback
and result. They preserve four payload slots and 144 logical payload bits. The
stateless Step module contains only combinational selection and arithmetic and
adds no storage or cycle. Ordinary typed calls and queue connections use the
existing frontend and common IR without a new feedback primitive.

## Timing and priority

Resident feedback has priority over the source head. Continuation advances even
while the result queue is full; only a completed exit waits for output capacity.
A resident feedback exit cannot process a new source head on that same edge.
External offers may still enter free source slots; a full source cannot replace
its head until that head is actually consumed.

With initially empty queues, E0 accepts a token. remaining 0 exits at E1 and first
retires at E2. For remaining=n>0, updates occur at E1 through En, exit pushes at
E(n+1), and first retirement is E(n+2). A stalled exit retains its completed value.
All queues explicitly use downstream_pop; the feedback slot consumes/replaces
itself on a continuation. Its input-ready is not fed into its own take decision,
so no combinational ready cycle or extra buffer is introduced.

Held/falling clocks and discarded proposals do not advance the loop. Rising
reset drops every occupied token. Invalid result data is packed zero. The compiler
generates hidden clock/reset pins driven by the host/SystemRunner, and the shared
Runtime manages Work/Xfer and whole-system discard.

## Preserved scope

The old feedback implementation also maintained an iteration count and a 1024
guard. For this exact design, initial remaining is 0..15 and every accepted
continuation decrements it; the guard is unreachable. This rewrite omits that
unobservable bookkeeping under the reviewed proof.
It does not implement arbitrary Python while loops or claim 1024-limit failure
coverage. The original payload capacity and edge timing remain required.

No-update exits preserve all input value/known/Z planes. After an arithmetic
update, unknown value bits produce the accepted computed-X value: masks and
known bits are exact, computed-X latent bits unspecified, and arithmetic emits
no Z. Unknown remaining controls a transfer and fails when effective, with no
partial commit; changing an external offer cannot repair a poisoned queued head.
Reset is required after terminal executor failure. These current failure rules
are not a claim of universal parity with legacy two-state/procedural behavior.

## Build and verification

```sh
cmake -S examples/feedback_pipeline -B /absolute/build/feedback_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/feedback_pipeline --parallel 4
ctest --test-dir /absolute/build/feedback_pipeline --output-on-failure --no-tests=error
```

Independent tests must check all remaining values, wrap, feedback priority,
four-slot saturation, progress despite result blockage, stable blocked exit,
no same-edge exit/source processing, exact edge latency, holds, busy reset,
discard/reprepare and observed token conservation. Native workers 1/2, known
Verilator and genuine Icarus execute the generated model. The current standalone module, system and four-state tests pass 3/3. There are 7,527 known and7,607
four-state Work rows, including 320 known tokens, 408 four-state tokens and
directed timing/backpressure histories.

Known history accepts 349 tokens, retires 341 and reset-drops 8; four-state history
is 437/429/8. Both drain empty and reach four occupied slots. Every initial
remaining value 0..15 has an isolated exact-latency check. Native 34 terminal
cases per worker and 18 isolated RTL failures cover selected unknown remaining
and protocol controls; queued unknown data waits safely behind resident feedback
until it becomes the selected control. Two owner probes verify whole-system
discard/reprepare and actual retained-token retirement after a discarded Reset.
The finite runner limit is 12,000 epochs per successful history.

## Generated system usage

`bench.py` exports `example_feedback_pipeline.bench.ExerciseFeedbackPipeline`. Compile
`feedback_pipeline.py`, then `bench.py`, import the published DUT interface, and link
that explicit system root through the public compile/link/emit flow. Run
`pycircuit run examples/feedback_pipeline --target cpp --cycles 3763` or select
`--target verilog`. Imported records use their original nominal declarations
and supply every field explicitly.

The bench stores 1,051 immutable inclusive intervals in one typed
Table. Each `Scenario` begins with `end: u64`, followed by the original
valid/value/remaining/take and four expected output fields. Their widths remain
1/32/4/1/1/1/32/4, making each complete interval row 140 bits. Zero-valued payload
fields use the existing implicit-zero Struct initialization; every endpoint is
explicit. Exact integer literals retain all payload and endpoint bits. The
author derived the rows from the frozen original bench's syntax tree without
importing or executing design code; that extraction is separate from the
independent preservation check.

Endpoints increase strictly. The existing `Table.first` query selects the first
row satisfying `phase <= row.end`, then indexes that row. The final endpoint is
18446744073709551615, so every known u64 phase has a matching interval. The
original constant tail begins at phase 3,699 and occupies the final interval
through 2**64-1.

The phase remains `bits[64]`, initialized to zero and advanced by the original
rule. Selection compares the complete phase, and arithmetic still wraps to zero,
restarting the original stimulus without resetting DUT state. All four assertions
keep their `phase < 3763` guard, empty else and original messages. All four logs
remain unconditional.

All original known-stream data edges and fixed expected values remain in the
regular-clock, resetless scenario. The complete 3,763-cycle run includes the
final observation.
The original DUT, native/RTL drivers, finite configuration, and any four-state,
reset/discard, latency and token-ledger matrices remain unchanged. Physical
held-level and midstream-reset scenarios still require those original module
oracles; this system does not claim complete physical-scenario equivalence.

## Verification and measured costs

The current standalone tests pass 3/3. The system completes all 3,763 cycles
(7,526 sampling epochs) with four source-check definitions and 30,104 observations
matching across native workers 1/2 and Verilator. The original module oracles
retain their 7,527 known and 7,607 four-state Work rows, including genuine Icarus
checks, reset/discard histories and token conservation. See the verified
[module excerpts and receipt](GENERATED.md).

An independent source check proves all 30,104 input/expected field values and
all 1,051 intervals over the complete known u64 phase domain, including the
constant tail and modulo wrap. Nineteen mutated candidates are rejected.
A finite system run does not execute the 64-bit rollover; source-domain proof
and runtime history provide separate evidence. Arbitrarily injected X/Z phase
values are outside this known-phase proof.

The source shrinks from 13,103 lines / 639,219 bytes to 1,116 lines / 90,403 bytes.
The phase measurements below run the original pipeline and then the interval
pipeline sequentially on the same machine, installed compiler and LLVM 22 C++
toolchain with `-O0`, using all 3,763 cycles in both versions. Each build/emission
and Verilog entry records one run.
Native runtime uses two additional paired warm runs in alternating order:
30.22/30.90 seconds originally and 31.61/31.57 seconds with intervals. The median
increases by 3.4%. Verilog build and runtime also increase, by about 1.29 and
0.20 seconds respectively in this run. This is a source/frontend improvement
with execution tradeoffs, not a simulation speedup. These are example-specific measurements, not a
cross-platform benchmark.

| Phase | Original conditional expressions | Interval table |
| --- | ---: | ---: |
| Compile bench | 21.49 s | 3.79 s |
| Link system | 14.24 s | 6.06 s |
| Emit C++ | 13.38 s | 5.96 s |
| Build C++ simulator | 4.11 s | 3.31 s |
| Run C++, one worker (paired warm median) | 30.56 s | 31.59 s |
| Emit Verilog | 10.56 s | 4.41 s |
| Build Verilog simulator | 2.35 s | 3.64 s |
| Run Verilog | 0.69 s | 0.89 s |

The final IR falls from 62,372,289 to 25,817,849 bytes. Generated C++ files total
10,817,925 → 7,894,464 bytes; generated Verilog files total
2,363,176 → 1,512,637 bytes. Both backends consume the same final IR. Complete
source-import and transformed artifacts are retained locally and reproduce the
published units byte-for-byte. No compiler API, DUT storage, scenario count,
cycle limit or timeout changes are required by this representation.

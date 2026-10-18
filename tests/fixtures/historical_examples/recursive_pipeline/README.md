# Three-increment queue pipeline

The issue #319 implementation uses named queues and three independent Rules,
one for each increment and transfer. The compiler derives resource admission
and full-queue replacement. The unchanged physical oracle passes native workers
1/2 and Verilator. The complete generated System bench also passes C++ workers 1/2 and RTL.
Generated excerpts below may describe the earlier candidate.

As in the accepted [queue_pipeline](../queue_pipeline/README.md) implementation,
the original `int` payload is unsigned fixed 64-bit hardware. Each of the
three pure additions wraps independently modulo 2^64 before its following
queue captures the result. For example, UINT64_MAX advances through 0, 1 and 2.
The queues retain eight complete tokens, totaling 512 payload storage bits,
in addition to their control metadata. `QueueResult` returns ready and valid
as `ac.u1`, and data as `ac.u64`; its packed result is 66 bits with data at
bits 0 through 63, valid at bit 64 and ready at bit 65.

Every queue declares depth 2, availability latency 1 and
`ready_policy="downstream_pop"`. Reads use the old committed head, with no
empty flow-through. A queue is ready when it has space or its available old
head will pop. The compiler connects each transfer to its downstream capacity;
the last queue takes the external `take` input. Thus full queues can replace
their heads simultaneously while retaining FIFO order and occupancy. Data
zero remains a valid token, and data does not determine readiness.

With downstream readiness throughout, E0 captures the external input into
the source queue, E1 captures its first increment, E2 its second, and E3 its
third. E4 is the earliest external retirement. Depth two provides capacity
without another mandatory latency stage. Stalls retain available tokens;
held clock levels do not transfer them. Rising synchronous reset empties all
four queues and can discard up to eight tokens. The host drives generated
physical clock/reset pins; Python declares no clock/reset or current/next API.
Existing whole-system Work/Xfer checking governs commit and discard.

Fixed-width addition uses the existing four-state arithmetic contract:
any X/Z input bit produces all-X computed data with no Z. The hidden value
plane of that computed unknown is unspecified. Unknown data remains a token
and does not suppress its valid signal.

## Current public flow

Compile `recursive_pipeline.py` as one source unit through `pycircuit compile`,
link that complete unit closure with `RecursivePipeline` selected as the root,
and use `pycircuit emit --target cpp` or `--target verilog` on the same verified
final artifact. The shared example CMake helper owns this flow and the generated
model build; host testbenches remain separate from this hardware source. Keep
generated outputs and build directories outside the source tree.

## Verified implementation

The current public compile/link/emit flow passed native workers 1 and 2 and
Verilator with 235 matching Work frames. Four independent logical deques check
E4 first retirement, eight-token capacity, backpressure, full replacement,
64-bit wrap, held clocks, reset and drain. The run observed 67 accepted,
51 retired and 16 reset-dropped tokens, ending empty. Coverage is deliberately
bounded; no exhaustive or four-state claim is made.

See [actual generated excerpts](GENERATED.md) and [verification inputs](GENERATED.json).

## Scenario bench

`bench.py` stores all 158 original scenarios in an immutable typed Table. Its
6-field, 132-bit `Stimulus` preserves field order, exact values and complete
input/expected payloads. Row 158 is all zero, retaining the helper fallback
for every known u16 phase from 158 through 65,535. That fallback is distinct
from the system counter, which holds at 157. The run still lasts 158 cycles.

This is a feed-forward chain of four D2 queues and three separate increments,
with capacity eight and E4 earliest retirement. The helper preserves that
trajectory; it does not substitute a feedback loop or single +3 stage.

The complete advance and system bodies remain unchanged, including ordered
arguments, all unmasked assertions/messages, log order and advance-before-check
registration. Independent source checks compare all 393,216 field values across
the known-u16 domain. Twenty complete native and RTL traces match all 1,264
observations and the terminal result after excluding only changed source-position
metadata, including native workers 1 and 2. Baseline and compact versions each
pass all 2 standalone tests. See the module [excerpts and receipt](GENERATED.md).

The unchanged physical driver checks 235 known Work samples. This example
has no registered four-state helper or RTL four-state macro; its acceptance stays
focused on known values. The arithmetic contract described above is not evidence
of a new four-state execution result for this example.

Regular-clock systems do not replace physical reset/held-clock histories, and
known-u16 source proof does not establish arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 1,034 lines / 25,423 bytes to
222 lines / 17,954 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 2.390 s | 0.499 s |
| Link system | 4.112 s | 0.912 s |
| Emit C++ | 3.056 s | 0.846 s |
| Emit Verilog | 2.807 s | 0.703 s |
| Build C++ | 0.960 s | 0.809 s |
| Build Verilog | 1.625 s | 1.390 s |
| First measured full C++ run | 0.683 s | 0.473 s |
| First measured full Verilog run | 0.562 s | 0.499 s |
| Warm C++ median, one worker | 0.124137 s | 0.016177 s |
| Warm Verilog median | 0.015221 s | 0.014681 s |
| Warm native peak RSS median | 2.97 MiB | 2.38 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 20,135,711 bytes | 3,517,440 bytes |
| Generated C++ total | 1,277,143 bytes | 1,012,787 bytes |
| Generated Verilog total | 280,657 bytes | 212,935 bytes |

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/recursive_pipeline --target cpp --cycles 158
pycircuit run examples/recursive_pipeline --target verilog --cycles 158
```

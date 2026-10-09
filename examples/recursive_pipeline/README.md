# Three-increment queue pipeline

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
head will pop. Each upstream queue takes its downstream queue's ready wire;
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

The separate `bench.py` system checks a finite regular-clock known-state scenario, including queue saturation, stalls, replacement, boundary values and final drain. This is partial system migration: the original independent drivers retain their full physical-clock, midstream-reset and four-state scenarios.

```bash
pycircuit run examples/recursive_pipeline --target cpp --cycles 158 --build-dir .pycircuit_out/recursive_pipeline/system-cpp
pycircuit run examples/recursive_pipeline --target verilog --cycles 158 --build-dir .pycircuit_out/recursive_pipeline/system-verilog
```

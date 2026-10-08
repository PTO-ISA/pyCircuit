# Two-picker issue queue

`issue_queue_2picker.py` rewrites the historical four-slot issue queue with
ordinary persistent `Slot` variables and one stateless rule. Each slot contains
one valid bit and eight data bits, for 36 state bits. `Slot()` initializes all
fields to zero. The source contains no clock, reset or register primitive API;
the compiler infers storage and the physical sampling/reset connections.

The first picker observes slot 0 and the second observes slot 1. Output values
come from old Q during Work, before any shift or insertion. Data is observable
even when its valid bit is clear. The first picker pops when
`s0.valid & out0_ready`; the second pops when
`s1.valid & out1_ready & pop0`, so it cannot pop independently. Input readiness
is `~s3.valid | pop0`. A full queue can therefore accept a replacement when the
first picker pops. `in_valid & in_ready` accepts at most one new byte.

The rule first shifts once for `pop0`, then once for `pop1`, and finally inserts
the accepted byte into the first invalid slot. A shift clears the last slot's
valid bit while retaining its data. The two shifts and insertion use pure
conditional values; all four final slot assignments are unconditional. This
preserves the historical value-merge behavior for unknown controls without
turning those controls into conditional storage enables.

Work computes the result and prepares the next slots. Successful whole-system
checking precedes Xfer, which commits the slots on the generated clock's rising
edge; reset restores all 36 bits to zero. A failed or discarded Work does not
commit the prepared state. A sampled result remains the Work result and is not
recomputed after commit.

The public source flow is `pycircuit compile` for this source unit,
`pycircuit link` with `IssueQueue2Picker` as the selected root, then
`pycircuit emit --target cpp|verilog` from the same verified final artifact.
Generated models use the typed DUT and shared SystemRunner; the host supplies
explicit physical clock/reset levels and a finite runner limit.

## Verified implementation

The current public compile/link/emit flow passed the shared example runner with
native workers 1 and 2 and Verilator: 177 matching Work frames. The independent
deque oracle covers capacity four, first/second-pop gating, stalls, full
replacement, held clocks, reset and drain. It observed 62 accepted, 54 retired
and eight reset-dropped tokens. Invalid payloads are normalized in the trace;
this bounded run does not claim exhaustive or four-state coverage.

See [actual generated excerpts](GENERATED.md) and [verification inputs](GENERATED.json).

The separate `bench.py` system checks a finite regular-clock known-state scenario, including queue saturation, stalls, replacement, boundary values and final drain. This is partial system migration: the original independent drivers retain their full physical-clock, midstream-reset and four-state scenarios.

```bash
pycircuit run examples/issue_queue_2picker --target cpp --cycles 344 --build-dir .pycircuit_out/issue_queue_2picker/system-cpp
pycircuit run examples/issue_queue_2picker --target verilog --cycles 344 --build-dir .pycircuit_out/issue_queue_2picker/system-verilog
```

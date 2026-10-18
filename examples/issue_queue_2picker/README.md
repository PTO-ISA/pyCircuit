# Two-picker issue queue

`issue_queue_2picker.py` implements a four-slot issue queue with
ordinary persistent `Slot` variables and one nested state Rule. Each slot contains
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

## Scenario bench

`bench.py` stores all 344 original scenarios in an immutable typed Table. Its
9-field, 30-bit `Stimulus` retains the original field order, including payloads
supplied or observed while valid is zero. Row 344 is all zero, preserving the
helper fallback for every known u16 phase from 344 through 65,535. This fallback
is distinct from the system counter, which holds at phase 343.

The complete system retains its ordered DUT inputs, 5 unconditional assertions,
6 unconditional observations and `advance(phase)` before `exercise()`.
The regular-clock run still lasts 344 cycles.

Invalid output data remains part of this system's contract. There are 26 rows
with nonzero expected output-0 data while valid is zero, and 295 such rows for
output 1. At phase 343 both valid bits are zero and both expected data bytes are
8. Neither payload assertion is masked. The physical driver's normalized invalid
trace fields are a separate policy and are not used to weaken these checks.

Independent source checks compare all 589,824 field values across the complete
known-u16 helper domain. Twenty complete native and RTL traces agree on all
4,128 observations and the terminal result after excluding only changed
source-position metadata; native workers 1 and 2 are included. Baseline and
compact versions each pass all 2 standalone gates. See the module
[excerpts and receipt](GENERATED.md).

The unchanged 177-frame physical driver retains picker gating, capacity,
replacement, held-clock, reset and conservation coverage. This example has no
independent four-state gate; the compact system does not establish such coverage.

The closed system covers a regular-clock known-state trajectory. Full physical
reset/held-clock scenarios remain with their independent module drivers;
known-u16 source equivalence does not prove arbitrary X/Z-phase equivalence.

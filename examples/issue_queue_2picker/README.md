# Two-picker issue queue

Current migration status and remaining work are tracked in the
[catalog](../catalog.json) and [migration inventory](../README.md#remaining-migration-work).
This design and its independent oracles are retained outside the current execution
set. Historical verification below is not a PASS for the current candidate.

`PickerStorage` owns four initialized `Slot` States, each containing one valid
bit and eight payload bits. Its `transact` Method snapshots committed old Q,
computes two dependent shifts and one insertion with ordinary local values,
and assigns each slot once. The source contains no clock/reset or register-pin
plumbing.

Picker 0 observes slot 0; picker 1 observes slot 1. Outputs always report old Q,
including payload bytes when their valid bits are zero. Picker 0 pops on
`slot0.valid & out0_ready`; picker 1 pops on
`slot1.valid & out1_ready & pop0`, so it cannot pop independently. Input readiness
is `~slot3.valid | pop0`; a full queue can accept one replacement when picker 0
pops.

The Method shifts once for `pop0`, once for `pop1`, then inserts an accepted byte
in the first invalid locally computed slot. A shift clears the last valid bit
while retaining its data. The original Struct-valued conditional expressions
remain intact, preserving the intended value-merge algorithm for unknown
controls. Replacing them with guarded state updates would change that contract.

`IssueQueue2Picker` retains the original four input arguments and explicitly
registers its calling Rule. The input-bearing physical root still needs the
transactional export boundary; no delayed result State or Queue substitutes for
its same-Work result.

Source compilation passes with the compiler's generic Struct-selection repair.
The original expression `old1 if pop0 else old0` remains intact. Full-history
System linking then exceeds the existing analysis-work budget during compact
dependency analysis (used 4,194,303, requested 4, limit 4,194,304). The complete
history and budget are retained. Native workers 1/2 and RTL System simulation
are **NOT RUN**; source compilation is not execution evidence.

## Full System history

The System owns one storage instance and explicitly registers `advance()` before
`exercise()`. The counter holds at 343, and all reads still observe old Q.
`exercise` selects the original row, calls the existing instance's Method, and
checks all five result fields without masking invalid payloads. All six logs
remain unconditional.

The initialized Table retains all 344 original `Stimulus` rows plus the all-zero
fallback row 344. Selection clamps to that fallback for known phases 344 through
65535, separately from the counter's hold behavior. There are 26 active rows
with nonzero invalid output-0 payloads and 295 for output 1. The final active row
expects both valid bits zero and both data bytes equal to 8.

Run the full generated System with an installed toolchain:

```sh
pycircuit run examples/issue_queue_2picker --target cpp --workers 1 --cycles 344
pycircuit run examples/issue_queue_2picker --target cpp --workers 2 --cycles 344
pycircuit run examples/issue_queue_2picker --target verilog --cycles 344
```

The existing CMake entry retains separate physical Module and System tests:

```sh
cmake -S examples/issue_queue_2picker -B /absolute/build/issue-queue -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
ctest --test-dir /absolute/build/issue-queue -R issue_queue_2picker_system \
  --output-on-failure --no-tests=error
```

## Retained physical verification

The unchanged 177-frame physical driver uses an independent deque oracle for
capacity four, dependent picker gating, stalls, full replacement, held clocks,
reset, drain, and token conservation. Its trace normalizes invalid payloads;
that policy never weakens the System's unmasked payload checks.

Physical native workers 1/2 and RTL are **NOT RUN** for this migration while the
original export boundary is unfinished. This example has no independent
four-state execution gate; the retained mux algorithm does not establish new
X/Z evidence. System adapter verification cannot replace physical reset and
held-clock histories.

[Generated excerpts](GENERATED.md) and [verification inputs](GENERATED.json)
remain historical records of the superseded frontend.

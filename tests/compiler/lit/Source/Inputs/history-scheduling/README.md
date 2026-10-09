# Finite dependency and persistent scheduling

These test-owned sources retain the complete finite scheduling algorithms from
`examples/agentic-circuit/pipelines/pyc_dependency_pipeline.py` and
`persistent_schedule.py` at baseline
`8887e6dec7b4cc530a9967c860dc6a224d79a4ab`. The original source files, runtime
implementation, and independent C++ tests are preserved under `originals/`.
Run `Source/source-historical-scheduling.test` through the existing compiler lit
entrypoint. Each source compiles independently, explicit linking selects its
root, and C++ and Verilog emission consume that same final verified artifact.

`DependencyPipeline` retains four resident entries, two resources, the original
four-bit sequence/predecessor/cost, one-bit resource and 16-bit value fields, and
sentinel 15. A predecessor is ready only while its old resident entry is Done.
Duplicate checks cover live resident keys; a retired key can be reused.
`PersistentSchedule` retains the original eight-bit sequence/predecessor/cost,
two-bit resource and 16-bit value fields, four entries, two resources and sentinel
255. Persistent seen/completed Tables retain keys 0 through 254 after retirement.
Key 255 and any seen-key reuse reject. The 256th physical Table element is unused;
it does not admit the reserved sentinel as a key.

Both modules preserve depth-four, latency-one input and completion queues.
Admission uses an old free entry, so a retiring entry is available only on the
following edge. Admission cannot issue on the same edge, and a newly completed
entry cannot retire on the same edge. Each resource selects its lowest-key ready
waiting entry. A running entry whose deadline is due permits another issue on
that resource while completion commits. Retirement selects the earliest old Done
deadline, then the lowest key. All selections and snapshots precede every state
proposal. The local 64-bit epoch advances on every successful rising edge,
including idle edges; issue overflow checks precede commit.

Cost, duplicate-key, reserved-key and resource checks apply only when an old input
head is available and an old resident slot is free. Full capacity therefore
defers an invalid input head, including on a retirement edge. The generic current
`source_check_failed` code carries the original `dependency_*` message. The module
gate preserves every successful low/high sample, failed suffix and explicit host
Reset operation from the independent oracle. Read-only committed-state snapshots
cover entry, epoch, history and FIFO storage plus clock history; failed checking
must commit none of them. C++ checks run with one and two workers. Verilator checks
use the existing generated Work/check/Xfer/Discard/Reset controls.

`scheduling_systems.py` exports the bare `@system` roots
`history_scheduling.scheduling_systems.pyc_dependency_pipeline` (32 cycles) and
`history_scheduling.scheduling_systems.schedule_v2` (20 cycles). They import the
original nominal DUT providers and author only finite data stimulus and
observations. Compiler-generated execution owns clock/reset controls. Their
complete public observations are compared against independent host mathematics
for C++ workers one/two and Verilator. The persistent system streams the original
three-token persistence example into an empty queue; the long resource-one token
holds a successor until its predecessor has left the resident table.

Each system sample requires exactly eight unique scalar events, their declared
Boolean/integer kinds and value ranges, and contiguous evaluation/commit epochs.
Successful comparisons cover public outputs. Committed-state snapshots prove
failure atomicity; they are not successful internal-state golden comparisons.

The historical C++ tests also preload three or four tokens into an input queue in
one host transaction. The current complete-token source queue has one push per
edge and initializes empty, so these three batch-preload histories remain
independent model/original-source oracles. They are not represented as fake
prefill or shorter counter systems. A custom-domain invalid-predecessor oracle is
model-only: with the original schedule sentinel 255, every eight-bit predecessor
is either an allowed key or the sentinel. A resource value of two is representable
by the schedule token, while the dependency token's one-bit resource cannot carry
it. These type limits do not change the original declarations or specialize the
DUT to a smaller default. No four-state or nightly result is claimed by this gate.

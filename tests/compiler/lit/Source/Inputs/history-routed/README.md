This source fixture restores the complete routed dependency graph through the
current compile/link/emit flow. `baseline/manifest.json` binds the exact inert
8887 source, b3e52 strong test files, and native block implementation. Retired
frontends are never executed. The original full-root tests checked deterministic
topology/common artifacts and native/RTL builds; they contained no end-to-end
runtime history for this graph. The independent runtime vectors are new coverage.

The active graph preserves the canonical 106-bit WorkItem, including opcode,
four routes, fourteen actual queues, a global eight-entry/four-resource dependency
scheduler, a four-input round-robin merge, and a 64-entry reorder buffer. Queue
depths in topology order are `16,4,16,8,8,8,8,1,1,1,1,8,8,1`, all latency one with
local occupancy capacity. The frontend adds one, branches add one through four,
and the final stage adds 100, modulo u64. Four original internal queue views and
the output remain public observations. Queue/state ownership remains hardware.

The independently reviewed native/spec scheduler contract retains live Done
entries as dependency readiness, uses absolute u64 epoch deadlines, issues the
lowest ready key per resource, and retires the earliest deadline/key. It does not
remember retired completion in a global bitmap. The reorder next key is u64;
all resident cells retain their other fields when valid clears. Admission,
completion, issue, retirement and queue capacity use old state. No same-edge
forwarding or freed-slot reuse is introduced. Deferred validation runs only when
old resident capacity permits admission; a failure rejects every state/queue
commit and stays latched until public host Reset.

The retired PYC lowering selected first slots for issue and retirement and used
the projected u8 reorder counter. Equivalence to that owner is not claimed.
Its countdown completes at one and preserves ordinary unit-epoch timing away
from overflow; there is no countdown off-by-one claim. Known typed route values
are u2 and therefore all fit four branches. An invalid known route four cannot
be manufactured by widening the original payload. Seeded near-u64-overflow and
control-reference vectors are retained independently but are not executions.

`run.py`, owned by `source-historical-routed.test`, compiles every source
independently in this order: `records.py`, `scheduler.py`, `merge.py`, `reorder.py`,
`routed_dependency_pipeline.py`, `routed_systems.py`, package `history_routed`.
All eleven independent module histories contain 1,792 attempts, run with C++
workers one/two and RTL on the same actual fourteen-queue graph. The checker
compares the complete 21,934-bit normalized before/after state, actual handshake
transfer counts, complete sink/observer payload deltas, deduplicated held-head
observations, and protected public old-Q samples. Failure additionally compares
raw physical storage, register/queue clock bookkeeping and native mask planes;
failed SDK sampling is rejected. Test-owned native headers change visibility
only, and RTL accesses state read-only. No expected ledger drives the DUT.

Two genuine closed roots import the same graph:
`history_routed.routed_systems.routed_dependency_pipeline` (81 cycles) and
`history_routed.routed_systems.routed_full_topology_backpressure` (727 cycles).
Each checks incoming capacity plus all five availability/payload views with 36
hardware assertions, emits ordered low/high logs, and runs both backends. The
full-capacity case includes held valid input while incoming capacity is false.
Literal phase inputs and checks come from the separately owned immutable vectors;
splitting their source functions bounds dependency analysis without shortening
any history or removing obligations. The other nine histories, faults and host
reset recovery remain module coverage, not executed closed-system roots.

The gate writes a receipt only after all checks pass and source, oracle, helper,
and Runtime hashes remain unchanged. It retains source-unit/final-IR/backend
stage hashes and exact commands. This is known-state coverage; it does not
establish the complete historical X/Z or physical clock-control matrix. A named
fixture or this README alone is not evidence of a passing run.

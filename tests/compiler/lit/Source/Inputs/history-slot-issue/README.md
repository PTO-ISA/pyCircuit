These test-owned fixtures preserve the historical mailbox and resident-only issue
algorithms through the current source-unit route. The six full original sources
and strong tests are inert under `baseline/`, with revision and SHA256 identities
in its manifest. The active gate never executes a retired compilation route.

The mailbox has two independent instances, each with a real depth-one,
latency-one input FIFO, distinct persistent valid and Event payload owners, and
a real depth-one, latency-one output FIFO. Capture reads the old empty slot;
release requires an accepted output. Release retains payload and cannot refill
on the same edge. The left core uses a global rule with explicit owners. The
right core preserves captured access through two nested binding rules and two
independently published ordinary storage modules. Each storage module lowers to
one real standard register; both share the compiler-inferred domain and the
whole-system commit/reset boundary. The nested bindings freeze the same old-Q
predicates and borrowed payload before proposing either owner.

This is a supported source realization of the compound slot algorithm. It does
not establish the original Slot object API, arbitrary persistent nested Python
functions, or an ordinary typed-Struct call to the original nested module form.
Contextual zero initializes the imported nominal payload types; their all-zero
initial state is unchanged. No compiled state is mutated by a host initializer.

The issue algorithm retains two real depth-two input FIFOs, two separate
resident slots, a four-entry Table, and a depth-one output FIFO. It captures
allocation while the table is full, installs only into an old invalid row, and
releases the allocation only on installation. Wakeup uses the old resident slot
valid bit, ignores its payload's `Wakeup.valid`, and writes each operand's
readiness from old valid/tag/readiness fields. Selection reads the old ready
rows and minimizes `(age,index)`; it clears selected validity even when the
output FIFO is full. Output capacity controls acceptance of the output token.
No wakeup-to-selection or allocation-to-wakeup forwarding is introduced.

The independent owner supplies complete literal histories under
`tests/compiler/oracles/history_slot_issue/`. All nine executable cases retain
159 epochs, with 54-bit mailbox and 250-bit issue state images. The native and
managed RTL harnesses observe actual FIFO contents, retained slot payloads and
all Table fields before Work and after commit. Transfer counters and received
records derive from actual control/head signals, then the independent checker
compares every row. Native uses the existing public SystemRunner and typed DUT
with workers one and two. RTL freezes the complete error permission before
commit. Read-only visibility copies change only access labels in emitted
headers; they do not establish a public state-inspection API. The owning audit
compares every native public log sample against the independent controls and
checks exactly two nested physical register owners and their inferred domains
in both emitted backends.

The original six-cycle mailbox drivers injected a committed token before their
first cycle. The executable current schedule adds an ordinary physical offer
edge and retains their complete six-cycle body as a separate seven-edge case.
The original issue driver initialized four heterogeneous Table rows through a
host API. Its complete thirty-edge history remains independent reference
evidence; the current forty-two-edge case reaches that seed through four real
allocations before running the thirty-edge body. These are explicit reset-
reachable scenarios, not claims that exceptional host initialization has been
restored. Additional independent histories preserve backpressure, old-state
selection, separate operand updates, full replacement and ignored wakeup flags.

Fixture registration and source emission are not passing simulation evidence.
The owning lit gate writes `receipt.json` only after all full histories pass and
its source/helper/runtime identities remain unchanged.

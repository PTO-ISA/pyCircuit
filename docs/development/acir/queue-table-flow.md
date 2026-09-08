# Queue and Table flow recording

Decision 0228 adds opt-in dataflow observation to Agentic Circuit/gfsim. It does
not change rule semantics, the Queue/Table commit barrier, activation, or the
CycleAwareSignal authoring contract. PYC/Verilator recording is outside this lane.

## Producer and viewer boundary

`agentic-circuit run --record-replay` adds `execution.pyctrace` to the run
publication and its output hashes. The immutable run manifest stores
`record_replay: true`; replaying that manifest retains the option. The default
run has no trace artifact. Recording does not turn a failed simulation into a
successful run: the run result remains authoritative for termination.

For source-checkout CLI validation, rebuild and install the native toolchain
from the same checkout when tests select an installed prefix through
`AC_GATE_TOOLCHAIN_ROOT`. Building only the `gfsim` target updates the build
tree, while CLI subprocesses can still load the prefix's older Python extension
and link its older runtime libraries. After a native recording change, run
`cmake --build <build-dir> --target install` before the CLI recording test.
An older runtime can reject `record_replay` with an invalid closed-envelope
diagnostic even though the current Python frontend accepts the option.

For manually driven generated QueueGraph models, register the model before
starting a recording:

```cpp
auto rows = model.dispatch_rows();
gfsim::ReplaySession session(path, rows);
model.registerObservations(session);
session.start();
// Direct driver: wrap each complete global Work/Arbitrate/Probe/Commit barrier.
session.begin(epoch);
// ... execute the original driver ...
session.end();
session.finish();
```

Direct host Queue transfers need the same boundary. Proposals may precede
`begin`, but committed state must change only inside the wrapped barrier.
For a model driven by `SimSystem`, call `session.attach(system)` after `start`;
the system then supplies boundaries. `finish` and destruction detach the
observer, including the system attachment. Objects and the system must outlive
the session. Do not move observed objects or replace their observer while the
session is active. Recording must finish before resetting or resuming an
unobserved execution; reset during recording fails before Queue/Table mutation.

pyCircuit owns only recording, the format contract, and producer tests. A viewer
is a separately packaged consumer; HTML/CSS/JavaScript, layout, browser tests,
and presentation dependencies do not belong in the framework distribution.
The viewer is tracked at `third_party/circuit-flow-viewer` as an independent
package and is not bundled in framework releases. Its CLI is:

```text
circuit-flow-viewer render execution.pyctrace --output replay.html
```

## Source names and display paths

Frontend `@ac.rule` declarations supply operation display names independently
of result variables. The optional `ac.source_name` string survives nested-rule
lifting, static specialization and rule lowering into QueueGraph plans.
Rule, firing, transform and instance verifiers reject empty or non-string
source labels. Existing stable IDs, object IDs and canonical paths retain their
identity roles; frozen integrity fingerprints still cover the new attributes.

Generated observation registration installs display metadata before
`ReplaySession.start()`. The trace topology adds `display_name`, `display_path`
and `display_parent_path`. The session snapshots these labels at start; changing
labels during recording is unsupported. No event fields or PYC6TRC3 framing
change. Objects without source metadata use their existing names.

Module display segments use the original function name and a zero-based index
among same-named calls in the same parent, including different static parameter
specializations. The root uses the selected system function name. Compiler-only
state scopes are transparent. Repeated rule calls keep the same title and gain
indexed path segments. For example, `pair/accumulator[0]/advance[1]` identifies
one call for display; source reordering may change this path.

Queue/Table names remain unchanged. The viewer groups cards by display parent
and uses source names for operation headings, without decoding internal symbol
strings. Clicking a heading exposes the original name/path and nearest module's
`module_parameters`, retained as exact textual MLIR static attributes for
inspection. Explicit instance aliases are not part of this interface.

## Runtime and adapter responsibilities

The optional `StateObserver` interface in `state_observation.h` has no file I/O,
codec, or replay-value dependency. `SimObject` holds one nullable observer
pointer. Dispatch supplies the executing owner/phase with scope cleanup, and
`SimSystem` supplies commit boundaries. The existing `ObservationRecorder`
proposal/event contract is separate and unchanged.

Queue notifies accepted/published push/pop proposal positions, consumed entries,
actual delayed arrivals, enqueues and transfer completion. Table notifies reads
and paired before/after writes around the actual replace/merge. Notifications
are synchronous read-only views: their data pointers and field spans must not
escape the callback. Observers must not mutate components, re-run policies,
perform nested dispatch or supply arbitration/backpressure decisions. Recording
errors propagate explicitly; an interrupted barrier never becomes a complete
record. The disabled path does not allocate recording metadata or encode values.

`ReplaySession` owns codecs, token/owner vectors, snapshots, registration and the
file writer. It follows Queue notifications rather than predicting Queue
capacity, readiness or commits. Its sorted resource registration preserves token
initialization order. Table snapshots use a read-only committed view so snapshot
collection does not manufacture read events.

Generated models expose a template `registerObservations(registry)` method.
It registers typed Queue/Table resources and connections at the model assembly
layer, including nested instances, control inputs and feedback queues; primitive
classes carry no topology methods. Reusable module registration receives its
port references from its parent without adding stored copies of those ports.
Handwritten model assembly uses the same interface:

```cpp
session.add(input);     // SimQueue<T>, not a type-erased SimObject reference
session.add(output);
session.add(state);     // SimTable<Entry>
session.connect(worker, {&input}, {&output}, {&state});
```

Dispatch rows enumerate identities, not typed state coverage. All Queue/Table
resources intended for observation must be registered with their concrete type
before `start`. A successful registration validates the sample entry codec;
unknown observed data resources fail explicitly when accessed. Other components
have topology and empty snapshots; internal buffers, memory storage, cursors and
sink history are not exported. New components using registered Queue/Table
state need only assembly registration, not recording code in their policies.
Private C++ state is allowed but is outside this observation contract.

`ValueCodec<T>` is the external payload extension point in `replay_value.h`.
Generated specializations define `encode(const T&)`, `fields()` and `flat`
outside payload structs. Scalar codecs preserve exact integers, declared widths,
and float bits. A handwritten payload can be adapted externally without adding
members to the type. Unsupported payloads remain usable when recording is off.

## Observation contract

The manifest enumerates object IDs, names, paths, connections, and Queue/Table
entry descriptors. `visual` identifies a Queue or Table. `fields` preserves the
entry declaration order; typed sample `entry` supplies scalar widths and types.
`flat: false` identifies generated nested records or packed aggregates. Recursive
nominal records are encoded by their generated codecs; the independent viewer
groups their fields under collapsible headers and preserves exact widths in
selection details. Its row and field filters only affect presentation; the full
state continues to advance atomically. The independent viewer documents its
controls in `third_party/circuit-flow-viewer/README.md` in the source checkout. Other
objects provide topology with empty state snapshots. The earlier local
prototype's extra private-state fields are no longer produced; existing trace
files remain readable by independent viewers.

An initial snapshot contains committed Queue contents, delayed contents with
ready times, token IDs, and all Table rows. A commit record contains changed
objects' complete `before` and `after` images. This includes all rows when any
row of a Table changes. The end record contains the final projection and status.

The recorder buffers real execution observations and publishes only these data
operations at a successful global boundary:

- `enqueue`: new Queue-local occurrence, token ID, value and ready time;
- `dequeue`: consumed occurrence, token ID and value;
- `ready`: delayed occurrence becoming readable, retaining its token ID;
- `state_read`: observed row/index/value in the committing operation's Work;
- `state_write`: row/index, write footprint, actual before/after merge result.

Events carry resource `object`, executing `owner`, and an `operation` identity
scoped by commit batch. Queue/Table hooks preserve the proposing writer until
commit. The runtime executes at most one firing per component per global
barrier; repeated reads are retained within that operation. External proposals
and time-driven readiness use the invalid ObjectId as the external owner.
Failed candidates and reservation diagnostics are not presentation events.

Token identity is observation metadata; it never changes the payload. Equal
values are distinct tokens. A new enqueue gets a new identity, including when
an operation transforms or forwards an input to another Queue. Connections
express participation in an operation, not a field-level dependency or proof
that the output is the same token as the input. Long-lived component buffers do
not establish cross-barrier lineage in this version.

## Binary format, version 1

All framing integers are little-endian. The file begins with eight bytes
`PYC6TRC3`, u32 container version `3`, and u32 extension flags `1`. Each chunk is
u32 payload length, u32 type, and that many payload bytes. Queue/Table flow uses
chunk type `0xAC01`. This extends the container; it does not rename the trace
magic or reuse waveform records.

Each payload is a logical value with a one-byte tag:

| Tag | Representation after tag |
| --- | --- |
| 0 | Null; no bytes |
| 1 | Boolean; one byte, 0 or 1 |
| 2 | Integer; u32 width (1–64), u8 signed flag, u64 raw bits |
| 3 | UTF-8 string; u32 byte length followed by bytes |
| 4 | Array; u32 count followed by tagged values |
| 5 | Object; u32 count followed by string keys (length + bytes) and tagged values |
| 6 | Float64; eight raw IEEE bytes |

Every record is an Object containing `kind`, monotonic `sequence`, `batch`,
`time`, and `delta`. Record order is optional `source` attachments, `manifest`,
`initial`, repeated `barrier_begin` / zero or more `event` / `commit`, then `end`.
The manifest has `format: agentic-circuit-replay` and typed integer `version: 1`.
Object-state dictionaries use decimal ObjectId strings as keys. Token IDs are
recording-local u64 values. Integers must remain exact in external consumers;
JavaScript numbers cannot represent all recorded u64 values.

Only a closed commit updates reconstructed state. Consumers check sequence,
barrier pairing, before-images and the final projection. An interrupted or
corrupt tail is reported explicitly and never applies a partial commit. No
sampling, compression, or silent truncation is performed; u32 chunk framing and
serializer errors fail explicitly. Recording must finish before model reset;
reset replay remains outside version 1. Nested-record visualization is verified
by the independent viewer without changing the version-1 wire format.

## Validation

Producer evidence covers native per-boundary projections independent of the
recorder, equal-valued tokens, delayed readiness, atomic multi-owner writes,
backpressure/retry, code-generated field ordering, and ROB scan/activation
committed-state equivalence. Viewer evidence separately covers reconstruction,
partial files, offline HTML generation, browser navigation and animation.


The DavinciOO ROB follow-up verifies recursive FlowKey/epoch/instruction/ROB
records and enum-valued fields, including 32-bit fault codes and integers above
2^53. Five expected-result scenarios compare every scan/activation boundary and
normal/recorded projections. See
[the ROB evidence](../../gates/logs/20260908-davincioo-rob/summary.md).

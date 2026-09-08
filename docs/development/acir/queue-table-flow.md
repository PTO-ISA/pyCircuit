# Queue and Table flow recording

Decision 0223 adds opt-in dataflow observation to Agentic Circuit/gfsim. It does
not change rule semantics, the Queue/Table commit barrier, activation, or the
CycleAwareSignal authoring contract. PYC/Verilator recording is outside this lane.

## Producer and viewer boundary

`agentic-circuit run --record-replay` adds `execution.pyctrace` to the run
publication and its output hashes. The immutable run manifest stores
`record_replay: true`; replaying that manifest retains the option. The default
run has no trace artifact. Recording does not turn a failed simulation into a
successful run: the run result remains authoritative for termination.

For manually driven generated QueueGraph models, construct
`gfsim::ReplaySession(path, rows)`, call `start()`, and wrap each complete global
Work/Arbitrate/Probe/Commit barrier in `begin(epoch)` / `end()`. Direct host
Queue transfers need the same boundary. Call `finish()` after the last complete
barrier. Objects must outlive the session. Proposals may precede `begin`, but
committed state must change only inside the wrapped barrier. A caller attaching
a session to `SimSystem::setReplayRecorder` must detach it before destruction.

pyCircuit owns only recording, the format contract, and producer tests. A viewer
is a separately packaged consumer; HTML/CSS/JavaScript, layout, browser tests,
and presentation dependencies do not belong in the framework distribution.
The local experimental viewer lives at `third_party/circuit-flow-viewer` and is
not part of the framework commit or release. Its CLI is:

```text
circuit-flow-viewer render execution.pyctrace --output replay.html
```

## Observation contract

The manifest enumerates object IDs, names, paths, connections, and Queue/Table
entry descriptors. `visual` identifies a Queue or Table. `fields` preserves the
entry declaration order; typed sample `entry` supplies scalar widths and types.
`flat: false` identifies generated nested records or packed aggregates that this
first viewer cannot render. Other objects provide topology, without a claim to
complete private-state coverage.

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
reset replay and nested-entry visualization are outside version 1.

## Validation

Producer evidence covers native per-boundary projections independent of the
recorder, equal-valued tokens, delayed readiness, atomic multi-owner writes,
backpressure/retry, code-generated field ordering, and ROB scan/activation
committed-state equivalence. Viewer evidence separately covers reconstruction,
partial files, offline HTML generation, browser navigation and animation.

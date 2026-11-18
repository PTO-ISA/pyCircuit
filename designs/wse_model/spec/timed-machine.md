---
doc_id: WSEMIG-DOC-TIMED-MACHINE
status: draft
authority: normative
owner: wse-calendar
---

# Data-carrying timed communication profile

## Modeled time and real memory {#WSEMIG-TIME-EXEC-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=noc status=draft -->

`TimedWseMachine` MUST consume runtime commands and actual memory images. It MUST
read source bytes at modeled read completion and publish destination bytes at
modeled write completion. The final destination write and receive-ledger commit
MUST belong to the same transaction. WAIT MUST observe the shared receive
ledger, not an externally calculated completion time.

The model is a discrete-event behavioral machine. Its `model_time` counts
2 GHz NoC ticks. gfsim cycles maintain queues, traverse routes, and process bytes
at a fixed modeled instant; they MUST NOT be counted as additional NoC time.
No event may move modeled time backwards. Source snapshot and landing microsteps
MUST finish before another modeled event changes their visible storage.

This is a separate execution profile from the serialized `WseMachine`.
It models communication and endpoint storage, not compute instructions.

## Runtime program ownership {#WSEMIG-TIME-PROGRAM-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=noc status=draft -->

SEND (8) accepts one independent packet into a finite per-source queue. COPY (4)
uses the same transfer engine but blocks until its source queue drains through
that packet. Each source has one active packet and a one-flit injection holding
buffer. Further source flits wait until all first-hop branches accepted the
previous flit. This is an explicit conservative model resource limit.

BARRIER (9) drains all pending transfers, adds the configured alignment and arm
budgets, and establishes a new Calendar origin. END also requires drained
queues and in-flight events. The interpreter's zero-modeled-time control work
MUST NOT delay later unblocked SEND instructions merely because queue maintenance
uses solver steps. Queue capacity and blocking commands can delay issue.

The machine MUST NOT recognize AllGather, 32 participants, row/column phase names,
fixed addresses, or program-counter ranges. The AllGather adapter converts its
COPY block to SEND and explicitly inserts one BARRIER after each POST block.
Original source/route/sequence/addresses/lengths remain runtime inputs.

## Network and endpoint resources {#WSEMIG-TIME-RESOURCES-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=noc status=draft -->

A packet consists of ceil(payload_bytes / configured_payload_per_flit) flits.
The current profile uses 64-byte wire flits with 52 useful bytes. Each directed
link serves at most one flit per NoC tick. Accepted flits arrive after the
configured hop latency; forwarding does not wait for the complete packet.
Multicast branches contend independently for their outgoing links. Endpoints
accept at most one landing flit per tick across their incoming directions.

Each directed link has eight runtime-loaded Calendar slot masks. A mask names
permitted source nodes. Closed slots delay link requests to a permitted slot.
The default image assigns horizontal slots by source column and vertical slots
by source row; this mapping lives in host configuration, not in the hardware.

Independent read/write ports are modeled per node. An endpoint service duration
is first-beat latency plus the remaining beats at the configured core/NoC clock
ratio, rounded upward to NoC ticks. Source endpoints read a complete packet
before injection. Destination endpoints receive a complete packet before write
service. Fixed TX/RX crossing and packet-completion guard budgets are explicit.

Before transmission, the model MUST reserve receive bytes and identity slots for
new packets. Capacity exhaustion is a model-resource error. The event heap is
bounded; exhaustion MUST be reported, never silently drop events. Simultaneous
events use stable insertion order. Unsynchronized overlapping memory accesses
are ordered by these events; programs requiring ordering use completion waits.

A withheld landing permit delays actual destination commit and modeled completion.
Other events can advance. This profile retries commit at the next NoC tick;
it does not model a silicon-specific credit-return pipeline.

## Explicit assumptions and acceptance {#WSEMIG-TIME-VER-001}
<!-- ndf: kind=verification modality=must refinement=L3 domain=verification status=draft verifies=WSEMIG-TIME-EXEC-001,WSEMIG-TIME-RESOURCES-001 -->

Every result MUST retain the complete parameter profile and configuration hash,
program and data hashes, compiler/build identity, model ticks, event trace,
host runtime, and actual byte-comparison result. Pending parameters stay pending;
reported latency is a model prediction and not a measured silicon latency.

Independent checks MUST cover a one-hop transfer, disjoint simultaneous sources,
two sources sharing a directed link, a multi-flit two-hop pipeline,
a branching multicast including node 33, Calendar gating, and a controlled
landing stall. Hop-latency variation changes only that parameter. AllGather
acceptance MUST run the actual PTO-generated program and compare all destination
bytes with original source concatenation. Column source reads must follow the
required row writes and the explicit barrier. Formula-only projected timestamps
MUST NOT replace execution events or supply DUT outputs.

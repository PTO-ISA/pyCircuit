# MEM.MIF.TXN — Transaction

- Source candidate: `DAV-MEM-MIF-TXN-0001`
- Hardware hierarchy: **H3**, within H1 `MEM` / H2 `MIF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/mem/mif/txn.py`
- Current design-program execution status: **not implemented**. External source evidence is recorded separately.

Sole transaction-generation/downstream-ID coordinator and terminal response matcher across L2/BHU/NOC.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| txn_fragment_req | TxnFragmentReq | REQ-qualified fragment | proposed |
| l2_resp | L2Resp | L2 terminal/retry response | proposed |
| bhu_resp | BhuResp | Lower-memory terminal/retry response | proposed |
| txn_cancel_req | TxnCancelReq | Generation-qualified cancel/discard request | proposed |
| txn_terminal_ack | TxnTerminalAck | Consumer accepted terminal response | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| txn_fragment_ack | TxnFragmentAck | Allocated transaction/downstream ID acknowledgement | proposed |
| l2_req | L2Req | Qualified L2 request | proposed |
| bhu_req | BhuReq | Qualified lower-memory request | proposed |
| txn_terminal_resp | TxnTerminalResp | Matched terminal data/ack/retry/fault | proposed |
| txn_cancel_ack | TxnCancelAck | Cancel/drain result | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- TxnTable keyed by MemTxnKey
- Source fragment/downstream ID/generation/route/state
- Retry/terminal/cancellation mode
- Downstream free-ID array with generations

## Required capabilities to verify

- Associative Table and free-ID Array
- Multiple input/output Queue pairs
- Atomic allocate-row-plus-ack and send state transition
- Generation-safe response match
- PYC/RTL state lowering

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Allocate reserves ID, creates row, and acknowledges atomically
- Send changes state only with downstream acceptance
- Response validates ID and generation
- ID frees only after terminal consumer accepts
- Canceled reads retain discard-on-return obligation
- Visible writes remain under ORD

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze downstream route split, table/ID capacities, retry classes, and exact cancel modes.

## Contributor closure

- [ ] Claim the candidate and identify its parent/containing state owner.
- [ ] Resolve disposition; aliases and contained state must not duplicate hardware.
- [ ] Link the relevant NDF L0 intent and L1 behavior to this L2 implementation.
- [ ] Freeze port payload fields/widths, producer/consumer, parent seam, state/reset and timing profile.
- [ ] Define functional branches, all-or-none effects, contention and cancel/recovery lifecycle.
- [ ] Link a minimal failing gate for each actual framework/primitive gap and merge that shared fix first.
- [ ] Implement the accepted owner and design-local expected-result tests.
- [ ] Prove backpressure, identity/generation, exactly-once effects and isolated instances in gfsim.
- [ ] Integrate into H2/H1 and record admitted PYC/RTL evidence or remaining boundary.

## Source evidence

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:371` — Detailed TXN packet.
- `docs/specification/davincioo/ndf-next/tile/memory.md:16` — MEM retains identity through terminal response.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.

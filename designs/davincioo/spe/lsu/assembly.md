# SPE.LSU — H2 assembly

NDF refinement: **L2 microarchitecture**. Scalar memory order, forwarding and replay.

Proposed source: `designs/davincioo/spe/lsu/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| memory_issue | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| store_data | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| memory_response | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| recovery_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| memory_request | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| scalar_completion | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| replay_request | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| recovery_ack | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |

## Composition contract

- Parent owns each child-to-child seam; child state has one owner.
- Preserve FlowKey, epoch/generation, payload and distinct completion meanings.
- Freeze shared-resource arbitration and request/response/cancel transactions.
- A composition is not one whole-core atomic rule; retain multi-cycle protocol state explicitly.
- Exact payload fields, widths, multiplicities, depths and timing are open until reviewed.

## Checklist

- [ ] Link the implementing NDF L2 detail to L1 behavior and L0 intent.
- [ ] Accept child/contained-state dispositions and instance geometry.
- [ ] Freeze external ports and a producer-consumer-seam table.
- [ ] Implement parameter propagation and specialization reuse.
- [ ] Run composition tests for independent backpressure, cancellation, isolation and quiescence.
- [ ] Record gfsim results and admitted PYC/RTL boundary.

## Included H3 candidates

- [DAV-SPE-LSU-FWD-0001](fwd.md)
- [DAV-SPE-LSU-L1D-0001](l1d.md)
- [DAV-SPE-LSU-LFB-0001](lfb.md)
- [DAV-SPE-LSU-LHQ-0001](lhq.md)
- [DAV-SPE-LSU-LIQ-0001](liq.md)
- [DAV-SPE-LSU-MDB-0001](mdb.md)
- [DAV-SPE-LSU-RPL-0001](rpl.md)
- [DAV-SPE-LSU-SCB-0001](scb.md)
- [DAV-SPE-LSU-STQ-0001](stq.md)
- [DAV-SPE-LSU-TLB-0001](tlb.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)

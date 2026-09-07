# MEM.MIF — H2 assembly

NDF refinement: **L2 microarchitecture**. Request, transaction, ordering and external memory interface.

Proposed source: `designs/davincioo/mem/mif/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| scalar_tile_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| external_response | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| cancel | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| cache_request | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| external_request | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| client_response | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| cancel_ack | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |

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

- [DAV-MEM-MIF-ATOM-0001](atom.md)
- [DAV-MEM-MIF-GM-0001](gm.md)
- [DAV-MEM-MIF-ORD-0001](ord.md)
- [DAV-MEM-MIF-REQ-0001](req.md)
- [DAV-MEM-MIF-RSP-0001](rsp.md)
- [DAV-MEM-MIF-TXN-0001](txn.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)

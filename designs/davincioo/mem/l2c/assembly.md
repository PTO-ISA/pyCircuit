# MEM.L2C — H2 assembly

NDF refinement: **L2 microarchitecture**. Streaming-cache tag/data leases and dependency return.

Proposed source: `designs/davincioo/mem/l2c/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| memory_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| refill_response | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| maintenance_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| cancel | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| refill_request | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| memory_response | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| maintenance_ack | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
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

- [DAV-MEM-L2C-ARB-0001](arb.md)
- [DAV-MEM-L2C-BHU-0001](bhu.md)
- [DAV-MEM-L2C-DATA-0001](data.md)
- [DAV-MEM-L2C-MROB-0001](mrob.md)
- [DAV-MEM-L2C-RET-0001](ret.md)
- [DAV-MEM-L2C-RWDB-0001](rwdb.md)
- [DAV-MEM-L2C-TAG-0001](tag.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)

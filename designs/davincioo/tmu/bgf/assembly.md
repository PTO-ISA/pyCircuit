# TMU.BGF — H2 assembly

NDF refinement: **L2 microarchitecture**. Mapped bank requests, arbitration and transport.

Proposed source: `designs/davincioo/tmu/bgf/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| client_bank_requests | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| access_cancel | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| bank_responses | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| access_cancel_ack | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |

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

- [DAV-TMU-BGF-ARB-0001](arb.md)
- [DAV-TMU-BGF-MAP-0001](map.md)
- [DAV-TMU-BGF-RQ-0001](rq.md)
- [DAV-TMU-BGF-WQ-0001](wq.md)
- [DAV-TMU-BGF-XBAR-0001](xbar.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)

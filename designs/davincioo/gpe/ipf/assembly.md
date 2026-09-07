# GPE.IPF — H2 assembly

NDF refinement: **L2 microarchitecture**. Tagged inter-PE routing.

Proposed source: `designs/davincioo/gpe/ipf/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| participant_ingress_packets | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| link_acceptance | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| participant_egress_packets | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| transport_completion | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |

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

- [DAV-GPE-IPF-ARB-0001](arb.md)
- [DAV-GPE-IPF-REQ-0001](req.md)
- [DAV-GPE-IPF-RSP-0001](rsp.md)
- [DAV-GPE-IPF-RTR-0001](rtr.md)
- [DAV-GPE-IPF-XBAR-0001](xbar.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)

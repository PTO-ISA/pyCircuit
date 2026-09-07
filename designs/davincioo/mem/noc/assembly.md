# MEM.NOC — H2 assembly

NDF refinement: **L2 microarchitecture**. Parameterized memory packet transport.

Proposed source: `designs/davincioo/mem/noc/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| tagged_ingress_packets | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| downstream_credit_or_acceptance | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| tagged_egress_packets | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
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

- [DAV-MEM-NOC-ARB-0001](arb.md)
- [DAV-MEM-NOC-BUF-0001](buf.md)
- [DAV-MEM-NOC-RTR-0001](rtr.md)
- [DAV-MEM-NOC-VC-0001](vc.md)
- [DAV-MEM-NOC-XBAR-0001](xbar.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)

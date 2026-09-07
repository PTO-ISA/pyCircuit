# SMT.XTD — H2 assembly

NDF refinement: **L2 microarchitecture**. Unresolved extended-context proposal.

Proposed source: `designs/davincioo/smt/xtd/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| TBD_producer_extension | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| TBD_consumer_extension | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| TBD_wakeup | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| TBD_lifecycle_ack | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |

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

- [DAV-SMT-XTD-CEXT-0001](cext.md)
- [DAV-SMT-XTD-LIFE-0001](life.md)
- [DAV-SMT-XTD-PEXT-0001](pext.md)
- [DAV-SMT-XTD-VID-0001](vid.md)
- [DAV-SMT-XTD-VTB-0001](vtb.md)
- [DAV-SMT-XTD-WAKE-0001](wake.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)

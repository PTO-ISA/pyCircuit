# GPE.GCU — H2 assembly

NDF refinement: **L2 microarchitecture**. Group context, arrival and all-participant completion.

Proposed source: `designs/davincioo/gpe/gcu/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| group_issue | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| participant_arrival | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| participant_done | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| abort | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| participant_issue | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| group_resolve | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| abort_ack | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |

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

- [DAV-GPE-GCU-CGI-0001](cgi.md)
- [DAV-GPE-GCU-CNT-0001](cnt.md)
- [DAV-GPE-GCU-GGC-0001](ggc.md)
- [DAV-GPE-GCU-LCMT-0001](lcmt.md)
- [DAV-GPE-GCU-PGI-0001](pgi.md)
- [DAV-GPE-GCU-WAKE-0001](wake.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)

# GPE — H1 assembly

NDF refinement: **L2 microarchitecture**. Participant-qualified group work and rendezvous.

Proposed source: `designs/davincioo/gpe/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| group_command | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| participant_arrival | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| local_done | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| cancel_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| participant_issue | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| movement_request | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| group_resolve | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
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

- [DAV-GPE-GCU-CGI-0001](gcu/cgi.md)
- [DAV-GPE-GCU-CNT-0001](gcu/cnt.md)
- [DAV-GPE-GCU-GGC-0001](gcu/ggc.md)
- [DAV-GPE-GCU-LCMT-0001](gcu/lcmt.md)
- [DAV-GPE-GCU-PGI-0001](gcu/pgi.md)
- [DAV-GPE-GCU-WAKE-0001](gcu/wake.md)
- [DAV-GPE-GMM-GLD-0001](gmm/gld.md)
- [DAV-GPE-GMM-GMV-0001](gmm/gmv.md)
- [DAV-GPE-GMM-ISS-0001](gmm/iss.md)
- [DAV-GPE-GMM-LCMT-0001](gmm/lcmt.md)
- [DAV-GPE-GMM-STGB-0001](gmm/stgb.md)
- [DAV-GPE-GMV-DRDY-0001](gmv/drdy.md)
- [DAV-GPE-GMV-GBUF-0001](gmv/gbuf.md)
- [DAV-GPE-GMV-REQ-0001](gmv/req.md)
- [DAV-GPE-GMV-RSP-0001](gmv/rsp.md)
- [DAV-GPE-IPF-ARB-0001](ipf/arb.md)
- [DAV-GPE-IPF-REQ-0001](ipf/req.md)
- [DAV-GPE-IPF-RSP-0001](ipf/rsp.md)
- [DAV-GPE-IPF-RTR-0001](ipf/rtr.md)
- [DAV-GPE-IPF-XBAR-0001](ipf/xbar.md)

[Architecture](../ARCHITECTURE.md) · [Complete inventory](../MODULE_CHECKLIST.md)

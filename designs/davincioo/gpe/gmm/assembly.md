# GPE.GMM — H2 assembly

NDF refinement: **L2 microarchitecture**. Grouped matrix staging and issue.

Proposed source: `designs/davincioo/gpe/gmm/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| group_matrix_command | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| participant_operands | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| local_done | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| abort | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| matrix_issue | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| operand_request | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| group_matrix_resolve | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
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

- [DAV-GPE-GMM-GLD-0001](gld.md)
- [DAV-GPE-GMM-GMV-0001](gmv.md)
- [DAV-GPE-GMM-ISS-0001](iss.md)
- [DAV-GPE-GMM-LCMT-0001](lcmt.md)
- [DAV-GPE-GMM-STGB-0001](stgb.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)

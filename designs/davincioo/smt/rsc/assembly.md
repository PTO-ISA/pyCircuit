# SMT.RSC — H2 assembly

NDF refinement: **L2 microarchitecture**. Resource quota and admission accounting.

Proposed source: `designs/davincioo/smt/rsc/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| reserve_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| release_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| quota_configuration | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| reserve_grant | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| release_ack | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| quota_status | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |

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

- [DAV-SMT-RSC-PART-0001](part.md)
- [DAV-SMT-RSC-QUOTA-0001](quota.md)
- [DAV-SMT-RSC-SHARE-0001](share.md)
- [DAV-SMT-RSC-THRSH-0001](thrsh.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)

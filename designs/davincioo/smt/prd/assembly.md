# SMT.PRD — H2 assembly

NDF refinement: **L2 microarchitecture**. Predicate versions and explicit mask identities.

Proposed source: `designs/davincioo/smt/prd/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| predicate_read | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| predicate_write | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| rename_recovery | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| predicate_value | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| predicate_write_ack | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| recovery_ack | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |

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

- [DAV-SMT-PRD-FFR-0001](ffr.md)
- [DAV-SMT-PRD-MASK-0001](mask.md)
- [DAV-SMT-PRD-PMAP-0001](pmap.md)
- [DAV-SMT-PRD-PREG-0001](preg.md)
- [DAV-SMT-PRD-RECON-0001](recon.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)

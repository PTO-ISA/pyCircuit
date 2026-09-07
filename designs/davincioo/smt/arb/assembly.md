# SMT.ARB — H2 assembly

NDF refinement: **L2 microarchitecture**. Independent per-stage fair admission.

Proposed source: `designs/davincioo/smt/arb/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| fetch_candidates | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| decode_candidates | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| commit_candidates | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| resource_availability | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| fetch_grants | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| decode_grants | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| commit_grants | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |

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

- [DAV-SMT-ARB-CSEL-0001](csel.md)
- [DAV-SMT-ARB-DSEL-0001](dsel.md)
- [DAV-SMT-ARB-FSEL-0001](fsel.md)
- [DAV-SMT-ARB-QOS-0001](qos.md)
- [DAV-SMT-ARB-RR-0001](rr.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)

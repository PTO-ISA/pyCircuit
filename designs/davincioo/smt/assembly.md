# SMT — H1 assembly

NDF refinement: **L2 microarchitecture**. Flow lifecycle, selection and resource admission.

Proposed source: `designs/davincioo/smt/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| launch_stop | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| flow_activity | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| resource_status | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| publication_candidate | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| flow_configuration | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| fetch_decode_grants | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| resource_grants | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| publication_grant | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |

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

- [DAV-SMT-ARB-CSEL-0001](arb/csel.md)
- [DAV-SMT-ARB-DSEL-0001](arb/dsel.md)
- [DAV-SMT-ARB-FSEL-0001](arb/fsel.md)
- [DAV-SMT-ARB-QOS-0001](arb/qos.md)
- [DAV-SMT-ARB-RR-0001](arb/rr.md)
- [DAV-SMT-PRD-FFR-0001](prd/ffr.md)
- [DAV-SMT-PRD-MASK-0001](prd/mask.md)
- [DAV-SMT-PRD-PMAP-0001](prd/pmap.md)
- [DAV-SMT-PRD-PREG-0001](prd/preg.md)
- [DAV-SMT-PRD-RECON-0001](prd/recon.md)
- [DAV-SMT-RSC-PART-0001](rsc/part.md)
- [DAV-SMT-RSC-QUOTA-0001](rsc/quota.md)
- [DAV-SMT-RSC-SHARE-0001](rsc/share.md)
- [DAV-SMT-RSC-THRSH-0001](rsc/thrsh.md)
- [DAV-SMT-THR-CTX-0001](thr/ctx.md)
- [DAV-SMT-THR-MODE-0001](thr/mode.md)
- [DAV-SMT-THR-SWI-0001](thr/swi.md)
- [DAV-SMT-THR-TID-0001](thr/tid.md)
- [DAV-SMT-THR-TPC-0001](thr/tpc.md)
- [DAV-SMT-THR-TSB-0001](thr/tsb.md)
- [DAV-SMT-XTD-CEXT-0001](xtd/cext.md)
- [DAV-SMT-XTD-LIFE-0001](xtd/life.md)
- [DAV-SMT-XTD-PEXT-0001](xtd/pext.md)
- [DAV-SMT-XTD-VID-0001](xtd/vid.md)
- [DAV-SMT-XTD-VTB-0001](xtd/vtb.md)
- [DAV-SMT-XTD-WAKE-0001](xtd/wake.md)

[Architecture](../ARCHITECTURE.md) · [Complete inventory](../MODULE_CHECKLIST.md)

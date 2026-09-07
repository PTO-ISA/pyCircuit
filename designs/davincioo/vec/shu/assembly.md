# VEC.SHU — H2 assembly

NDF refinement: **L2 microarchitecture**. Profile-defined shuffle, gather/scatter and permutation.

Proposed source: `designs/davincioo/vec/shu/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| shuffle_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| operand_data | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| cancel | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| operand_request | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| shuffle_result | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| resolve | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |

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

- [DAV-VEC-SHU-GTH-0001](gth.md)
- [DAV-VEC-SHU-PERM-0001](perm.md)
- [DAV-VEC-SHU-SCT-0001](sct.md)
- [DAV-VEC-SHU-SHF-0001](shf.md)
- [DAV-VEC-SHU-SORT-0001](sort.md)
- [DAV-VEC-SHU-TRANS-0001](trans.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)

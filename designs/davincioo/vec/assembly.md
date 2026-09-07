# VEC — H1 assembly

NDF refinement: **L2 microarchitecture**. Retained vector operations and result staging.

Proposed source: `designs/davincioo/vec/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| engine_issue | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| operand_data | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| cancel_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| write_ack | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| operand_request | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| staged_result | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| engine_resolve | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
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

- [DAV-VEC-SFU-DIV-0001](sfu/div.md)
- [DAV-VEC-SFU-EXP-0001](sfu/exp.md)
- [DAV-VEC-SFU-LOG-0001](sfu/log.md)
- [DAV-VEC-SFU-RECIP-0001](sfu/recip.md)
- [DAV-VEC-SFU-RSQRT-0001](sfu/rsqrt.md)
- [DAV-VEC-SFU-SQRT-0001](sfu/sqrt.md)
- [DAV-VEC-SHU-GTH-0001](shu/gth.md)
- [DAV-VEC-SHU-PERM-0001](shu/perm.md)
- [DAV-VEC-SHU-SCT-0001](shu/sct.md)
- [DAV-VEC-SHU-SHF-0001](shu/shf.md)
- [DAV-VEC-SHU-SORT-0001](shu/sort.md)
- [DAV-VEC-SHU-TRANS-0001](shu/trans.md)
- [DAV-VEC-TLOP-DEC-0001](tlop/dec.md)
- [DAV-VEC-TLOP-FSM-0001](tlop/fsm.md)
- [DAV-VEC-TLOP-INJ-0001](tlop/inj.md)
- [DAV-VEC-TLOP-UROM-0001](tlop/urom.md)
- [DAV-VEC-VEX-ACC-0001](vex/acc.md)
- [DAV-VEC-VEX-ALU-0001](vex/alu.md)
- [DAV-VEC-VEX-DBUF-0001](vex/dbuf.md)
- [DAV-VEC-VEX-EXP-0001](vex/exp.md)
- [DAV-VEC-VEX-RED-0001](vex/red.md)
- [DAV-VEC-VEX-SBUF-0001](vex/sbuf.md)
- [DAV-VEC-VEX-WB-0001](vex/wb.md)

[Architecture](../ARCHITECTURE.md) · [Complete inventory](../MODULE_CHECKLIST.md)

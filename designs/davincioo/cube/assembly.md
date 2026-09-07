# CUBE — H1 assembly

NDF refinement: **L2 microarchitecture**. Matrix operand staging, accumulation and writeback.

Proposed source: `designs/davincioo/cube/assembly.py`. No implementation is claimed.

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

- [DAV-CUBE-ACC-CVT-0001](acc/cvt.md)
- [DAV-CUBE-ACC-IACC-0001](acc/iacc.md)
- [DAV-CUBE-ACC-RND-0001](acc/rnd.md)
- [DAV-CUBE-ACC-SAT-0001](acc/sat.md)
- [DAV-CUBE-DFL-ABUF-0001](dfl/abuf.md)
- [DAV-CUBE-DFL-BBUF-0001](dfl/bbuf.md)
- [DAV-CUBE-DFL-CELL-0001](dfl/cell.md)
- [DAV-CUBE-DFL-CSCL-0001](dfl/cscl.md)
- [DAV-CUBE-DFL-LAY-0001](dfl/lay.md)
- [DAV-CUBE-DFL-RDB-0001](dfl/rdb.md)
- [DAV-CUBE-DFL-XPOS-0001](dfl/xpos.md)
- [DAV-CUBE-MMA-KACC-0001](mma/kacc.md)
- [DAV-CUBE-MMA-MAC-0001](mma/mac.md)
- [DAV-CUBE-MMA-PIPE-0001](mma/pipe.md)
- [DAV-CUBE-MMA-SYST-0001](mma/syst.md)
- [DAV-CUBE-WBK-FMT-0001](wbk/fmt.md)
- [DAV-CUBE-WBK-WBF-0001](wbk/wbf.md)
- [DAV-CUBE-WBK-WQ-0001](wbk/wq.md)

[Architecture](../ARCHITECTURE.md) · [Complete inventory](../MODULE_CHECKLIST.md)

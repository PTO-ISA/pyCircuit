# MEM — H1 assembly

NDF refinement: **L2 microarchitecture**. Tagged scalar/Tile memory and external service.

Proposed source: `designs/davincioo/mem/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| scalar_tile_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| external_response | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| maintenance_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| cancel_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| scalar_tile_response | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| external_request | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| maintenance_ack | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
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

- [DAV-MEM-COH-FLU-0001](coh/flu.md)
- [DAV-MEM-COH-INV-0001](coh/inv.md)
- [DAV-MEM-COH-SNP-0001](coh/snp.md)
- [DAV-MEM-COH-WB-0001](coh/wb.md)
- [DAV-MEM-L2C-ARB-0001](l2c/arb.md)
- [DAV-MEM-L2C-BHU-0001](l2c/bhu.md)
- [DAV-MEM-L2C-DATA-0001](l2c/data.md)
- [DAV-MEM-L2C-MROB-0001](l2c/mrob.md)
- [DAV-MEM-L2C-RET-0001](l2c/ret.md)
- [DAV-MEM-L2C-RWDB-0001](l2c/rwdb.md)
- [DAV-MEM-L2C-TAG-0001](l2c/tag.md)
- [DAV-MEM-MIF-ATOM-0001](mif/atom.md)
- [DAV-MEM-MIF-GM-0001](mif/gm.md)
- [DAV-MEM-MIF-ORD-0001](mif/ord.md)
- [DAV-MEM-MIF-REQ-0001](mif/req.md)
- [DAV-MEM-MIF-RSP-0001](mif/rsp.md)
- [DAV-MEM-MIF-TXN-0001](mif/txn.md)
- [DAV-MEM-NOC-ARB-0001](noc/arb.md)
- [DAV-MEM-NOC-BUF-0001](noc/buf.md)
- [DAV-MEM-NOC-RTR-0001](noc/rtr.md)
- [DAV-MEM-NOC-VC-0001](noc/vc.md)
- [DAV-MEM-NOC-XBAR-0001](noc/xbar.md)

[Architecture](../ARCHITECTURE.md) · [Complete inventory](../MODULE_CHECKLIST.md)

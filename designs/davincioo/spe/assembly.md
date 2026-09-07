# SPE — H1 assembly

NDF refinement: **L2 microarchitecture**. Scalar execution and block-order publication.

Proposed source: `designs/davincioo/spe/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| launch | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| memory_response | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| engine_resolve | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| recovery_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| memory_request | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| tile_command | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| architectural_event | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
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

- [DAV-SPE-BCTRL-BFU-0001](bctrl/bfu.md)
- [DAV-SPE-BCTRL-BIFU-0001](bctrl/bifu.md)
- [DAV-SPE-BCTRL-BIQ-0001](bctrl/biq.md)
- [DAV-SPE-BCTRL-BISQ-0001](bctrl/bisq.md)
- [DAV-SPE-BCTRL-BMDB-0001](bctrl/bmdb.md)
- [DAV-SPE-BCTRL-BROB-0001](bctrl/brob.md)
- [DAV-SPE-BCTRL-CBRG-0001](bctrl/cbrg.md)
- [DAV-SPE-BCTRL-F5-0001](bctrl/f5.md)
- [DAV-SPE-DTU-CTR-0001](dtu/ctr.md)
- [DAV-SPE-DTU-DBG-0001](dtu/dbg.md)
- [DAV-SPE-DTU-LTP-0001](dtu/ltp.md)
- [DAV-SPE-DTU-PMU-0001](dtu/pmu.md)
- [DAV-SPE-DTU-TMON-0001](dtu/tmon.md)
- [DAV-SPE-DTU-XCHK-0001](dtu/xchk.md)
- [DAV-SPE-IEX-AGU-0001](iex/agu.md)
- [DAV-SPE-IEX-ALU-0001](iex/alu.md)
- [DAV-SPE-IEX-BRU-0001](iex/bru.md)
- [DAV-SPE-IEX-CMDIQ-0001](iex/cmdiq.md)
- [DAV-SPE-IEX-CMDP-0001](iex/cmdp.md)
- [DAV-SPE-IEX-DIV-0001](iex/div.md)
- [DAV-SPE-IEX-E1-0001](iex/e1.md)
- [DAV-SPE-IEX-E2-0001](iex/e2.md)
- [DAV-SPE-IEX-E3-0001](iex/e3.md)
- [DAV-SPE-IEX-E4-0001](iex/e4.md)
- [DAV-SPE-IEX-E5-0001](iex/e5.md)
- [DAV-SPE-IEX-E6-0001](iex/e6.md)
- [DAV-SPE-IEX-FSU-0001](iex/fsu.md)
- [DAV-SPE-IEX-GPR-0001](iex/gpr.md)
- [DAV-SPE-IEX-I1-0001](iex/i1.md)
- [DAV-SPE-IEX-I2-0001](iex/i2.md)
- [DAV-SPE-IEX-P0-0001](iex/p0.md)
- [DAV-SPE-IEX-P1-0001](iex/p1.md)
- [DAV-SPE-IEX-S2-0001](iex/s2.md)
- [DAV-SPE-IEX-STD-0001](iex/std.md)
- [DAV-SPE-IEX-W1-0001](iex/w1.md)
- [DAV-SPE-IEX-W2-0001](iex/w2.md)
- [DAV-SPE-IEX-W3-0001](iex/w3.md)
- [DAV-SPE-IEX-WBA-0001](iex/wba.md)
- [DAV-SPE-IFU-BF0-0001](ifu/bf0.md)
- [DAV-SPE-IFU-BF1-0001](ifu/bf1.md)
- [DAV-SPE-IFU-BF2-0001](ifu/bf2.md)
- [DAV-SPE-IFU-BF3-0001](ifu/bf3.md)
- [DAV-SPE-IFU-BF4-0001](ifu/bf4.md)
- [DAV-SPE-IFU-BIM-0001](ifu/bim.md)
- [DAV-SPE-IFU-BPFIFO-0001](ifu/bpfifo.md)
- [DAV-SPE-IFU-BPU-0001](ifu/bpu.md)
- [DAV-SPE-IFU-BRQ-0001](ifu/brq.md)
- [DAV-SPE-IFU-CBTB-0001](ifu/cbtb.md)
- [DAV-SPE-IFU-F0-0001](ifu/f0.md)
- [DAV-SPE-IFU-F1-0001](ifu/f1.md)
- [DAV-SPE-IFU-F2-0001](ifu/f2.md)
- [DAV-SPE-IFU-F3-0001](ifu/f3.md)
- [DAV-SPE-IFU-F4-0001](ifu/f4.md)
- [DAV-SPE-IFU-FQ-0001](ifu/fq.md)
- [DAV-SPE-IFU-GHR-0001](ifu/ghr.md)
- [DAV-SPE-IFU-GHRQ-0001](ifu/ghrq.md)
- [DAV-SPE-IFU-IB-0001](ifu/ib.md)
- [DAV-SPE-IFU-IBTB-0001](ifu/ibtb.md)
- [DAV-SPE-IFU-IDATA-0001](ifu/idata.md)
- [DAV-SPE-IFU-IMMQ-0001](ifu/immq.md)
- [DAV-SPE-IFU-ITAG-0001](ifu/itag.md)
- [DAV-SPE-IFU-ITLB-0001](ifu/itlb.md)
- [DAV-SPE-IFU-LBUF-0001](ifu/lbuf.md)
- [DAV-SPE-IFU-LOOP-0001](ifu/loop.md)
- [DAV-SPE-IFU-MBTB-0001](ifu/mbtb.md)
- [DAV-SPE-IFU-NLP-0001](ifu/nlp.md)
- [DAV-SPE-IFU-PBTB-0001](ifu/pbtb.md)
- [DAV-SPE-IFU-RAFE-0001](ifu/rafe.md)
- [DAV-SPE-IFU-RAHQ-0001](ifu/rahq.md)
- [DAV-SPE-IFU-RAS-0001](ifu/ras.md)
- [DAV-SPE-IFU-SC-0001](ifu/sc.md)
- [DAV-SPE-IFU-TAGE-0001](ifu/tage.md)
- [DAV-SPE-IFU-TGTFIFO-0001](ifu/tgtfifo.md)
- [DAV-SPE-IFU-UBTB-0001](ifu/ubtb.md)
- [DAV-SPE-IFU-UCACHE-0001](ifu/ucache.md)
- [DAV-SPE-IFU-UTLB-0001](ifu/utlb.md)
- [DAV-SPE-LSU-FWD-0001](lsu/fwd.md)
- [DAV-SPE-LSU-L1D-0001](lsu/l1d.md)
- [DAV-SPE-LSU-LFB-0001](lsu/lfb.md)
- [DAV-SPE-LSU-LHQ-0001](lsu/lhq.md)
- [DAV-SPE-LSU-LIQ-0001](lsu/liq.md)
- [DAV-SPE-LSU-MDB-0001](lsu/mdb.md)
- [DAV-SPE-LSU-RPL-0001](lsu/rpl.md)
- [DAV-SPE-LSU-SCB-0001](lsu/scb.md)
- [DAV-SPE-LSU-STQ-0001](lsu/stq.md)
- [DAV-SPE-LSU-TLB-0001](lsu/tlb.md)
- [DAV-SPE-OOO-CMAP-0001](ooo/cmap.md)
- [DAV-SPE-OOO-CMT-0001](ooo/cmt.md)
- [DAV-SPE-OOO-D1-0001](ooo/d1.md)
- [DAV-SPE-OOO-D2-0001](ooo/d2.md)
- [DAV-SPE-OOO-D3-0001](ooo/d3.md)
- [DAV-SPE-OOO-DSAQ-0001](ooo/dsaq.md)
- [DAV-SPE-OOO-DSP-0001](ooo/dsp.md)
- [DAV-SPE-OOO-EXC-0001](ooo/exc.md)
- [DAV-SPE-OOO-FLS-0001](ooo/fls.md)
- [DAV-SPE-OOO-IQ-0001](ooo/iq.md)
- [DAV-SPE-OOO-ISQ-0001](ooo/isq.md)
- [DAV-SPE-OOO-MPQ-0001](ooo/mpq.md)
- [DAV-SPE-OOO-PCB-0001](ooo/pcb.md)
- [DAV-SPE-OOO-R0-0001](ooo/r0.md)
- [DAV-SPE-OOO-R1-0001](ooo/r1.md)
- [DAV-SPE-OOO-R2-0001](ooo/r2.md)
- [DAV-SPE-OOO-R3-0001](ooo/r3.md)
- [DAV-SPE-OOO-R4-0001](ooo/r4.md)
- [DAV-SPE-OOO-REN-0001](ooo/ren.md)
- [DAV-SPE-OOO-ROB-0001](ooo/rob.md)
- [DAV-SPE-OOO-S1-0001](ooo/s1.md)
- [DAV-SPE-OOO-S3-0001](ooo/s3.md)
- [DAV-SPE-OOO-SMAP-0001](ooo/smap.md)
- [DAV-SPE-OOO-SRF-0001](ooo/srf.md)

[Architecture](../ARCHITECTURE.md) · [Complete inventory](../MODULE_CHECKLIST.md)

from __future__ import annotations

from examples.pycircuit.linxcore_frontend_pipeline.linxcore_frontend_pipeline import (
    build,
)
from pycircuit import CycleAwareTb, Tb, testbench


@testbench
def tb(t: Tb) -> None:
    bench = CycleAwareTb(t)
    bench.clock("clk")
    bench.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    bench.timeout(32)

    # Keep every abstract external owner available. D3 is intentionally held
    # for one cycle at the end to prove that its payload is resident.
    bench.drive("flush", 0)
    bench.drive("lookup_ready", 1)
    bench.drive("fetch_result_ready", 1)
    bench.drive("ib_enqueue_ready", 1)
    bench.drive("d2_resource_ready", 1)
    bench.drive("d3_reservation_ready", 1)
    bench.drive("out_ready", 0)
    bench.drive("rob_tail_epoch", 0x5A)
    bench.drive("virtual_rid_base", 0x12)

    bench.drive("in_valid", 1)
    bench.drive("in_pc", 0x1000)
    bench.drive("in_stid", 1)
    bench.drive("in_pe_id", 2)
    bench.drive("in_packet_uid", 0x44)
    bench.drive("in_fetch_seq", 0x123)
    bench.drive("in_checkpoint_id", 0x77)
    bench.drive("in_epoch", 3)
    # Four instructions with lengths 2, 4, 6, and 4 bytes.
    line = bytes(
        [0x00, 0xA0]
        + [0x01, 0xB1, 0xB2, 0xB3]
        + [0x0E, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5]
        + [0x01, 0xD1, 0xD2, 0xD3]
    )
    bench.drive("in_line_data", int.from_bytes(line, "little"))
    bench.drive("in_line_bytes_valid", 16)
    bench.drive("in_fetch_fault", 0)
    bench.drive("in_block_start_mask", 0b0001)
    bench.drive("in_block_stop_mask", 0b1000)
    bench.drive("in_pred_taken_mask", 0)
    bench.drive("in_pred_target", 0x1010)
    for lane in range(4):
        bench.drive(f"in_opcode_id_{lane}", 0x20 + lane)

    bench.expect("in_ready", 1)
    bench.expect("stage__f0__valid", 1)
    bench.next()
    bench.drive("in_valid", 0)

    # F0->F1->F2->F3->F4/IB->D1->D2->D3.
    for stage in ("f1", "f2", "f3", "f4_ib", "d1", "d2"):
        bench.expect(f"stage__{stage}__valid", 1)
        bench.expect("out_valid", 0)
        bench.next()

    bench.expect("out_valid", 1)
    bench.expect("out_stid", 1)
    bench.expect("out_packet_uid", 0x44)
    bench.expect("out_valid_mask", 0b1111)
    bench.expect("out_uop_count", 4)
    bench.expect("out_group_count", 4)
    bench.expect("out_tail_epoch", 0x5A)
    bench.expect("out_reservation_token", 0x5A12)
    bench.expect("out_len_0", 2)
    bench.expect("out_len_1", 4)
    bench.expect("out_len_2", 6)
    bench.expect("out_len_3", 4)

    # Hold one extra complete cycle, then release the exact same D3
    # transaction. Drives are applied before the cycle's active edge, so the
    # ready transition belongs to the following cycle.
    bench.next()
    bench.expect("out_valid", 1)
    bench.expect("out_packet_uid", 0x44)
    bench.next()
    bench.drive("out_ready", 1)
    bench.next()
    bench.expect("out_valid", 0)

    bench.finish(at=16)


__all__ = ["build", "tb"]

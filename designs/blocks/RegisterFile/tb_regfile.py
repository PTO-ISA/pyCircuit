from __future__ import annotations

import sys
from pathlib import Path

from pycircuit import (
    CycleAwareTb,
    Tb,
    compile_cycle_aware,
    testbench,
)

_THIS_DIR = Path(__file__).resolve().parent
if str(_THIS_DIR) not in sys.path:
    sys.path.insert(0, str(_THIS_DIR))

from regfile import build  # noqa: E402


@testbench
def tb(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(64)

    nr = 10
    nw = 5
    ptag_count = 256
    const_count = 128
    mask64 = (1 << 64) - 1

    def const64(ptag: int) -> int:
        v = int(ptag) & 0xFFFF_FFFF
        return ((v << 32) | v) & mask64

    def read_expected(addr: int, storage: dict[int, int]) -> int:
        a = int(addr)
        if 0 <= a < const_count:
            return const64(a)
        if const_count <= a < ptag_count:
            return int(storage.get(a, 0)) & mask64
        return 0

    def drive_cycle(reads: list[int], writes: list[tuple[int, int, int]]) -> None:
        if len(reads) != nr:
            raise ValueError(
                f"tb reads length mismatch: got {len(reads)} expected {nr}"
            )
        for lane in range(nr):
            tb.drive(f"raddr{lane}", int(reads[lane]))

        for lane in range(nw):
            tb.drive(f"wen{lane}", 0)
            tb.drive(f"waddr{lane}", 0)
            tb.drive(f"wdata{lane}", 0)

        for lane, waddr, wdata in writes:
            if lane < 0 or lane >= nw:
                raise ValueError(f"tb write lane out of range: {lane}")
            tb.drive(f"wen{lane}", 1)
            tb.drive(f"waddr{lane}", int(waddr))
            tb.drive(f"wdata{lane}", int(wdata) & mask64)

    seq = [
        {
            "reads": [0, 1, 2, 3, 4, 5, 6, 7, 8, 127],
            "writes": [],
        },
        {
            "reads": [9, 10, 11, 12, 13, 14, 15, 16, 17, 18],
            "writes": [],
        },
        {
            "reads": [128, 129, 130, 200, 255, 7, 127, 0, 64, 12],
            "writes": [
                (0, 128, 0x1111222233334444),
                (1, 129, 0x5555666677778888),
                (2, 130, 0xDEADBEEFCAFEBABE),
                (3, 200, 0x89ABCDEF01234567),
                (4, 255, 0x0123456789ABCDEF),
            ],
        },
        {
            "reads": [128, 129, 130, 200, 255, 7, 127, 1, 2, 3],
            "writes": [],
        },
        {
            "reads": [7, 127, 128, 129, 130, 200, 255, 1, 2, 3],
            "writes": [
                (0, 7, 0xAAAAAAAAAAAAAAAA),  # ignored: constant PTAG
                (1, 127, 0xBBBBBBBBBBBBBBBB),  # ignored: constant PTAG
                (2, 128, 0x0BADF00D0BADF00D),
                (3, 129, 0x0102030405060708),
                (4, 130, 0xFFEEDDCCBBAA9988),
            ],
        },
        {
            "reads": [7, 127, 128, 129, 130, 200, 255, 4, 5, 6],
            "writes": [],
        },
        {
            "reads": [131, 132, 133, 134, 135, 128, 129, 130, 7, 127],
            "writes": [
                (0, 131, 0x13579BDF2468ACE0),
                (1, 132, 0xFEDCBA9876543210),
                (2, 133, 0x1111111122222222),
                (3, 134, 0x3333333344444444),
                (4, 135, 0xABCDABCD12341234),
            ],
        },
        {
            "reads": [131, 132, 133, 134, 135, 128, 129, 130, 7, 127],
            "writes": [],
        },
        {
            "reads": [255, 200, 130, 129, 128, 0, 1, 2, 3, 4],
            "writes": [
                (0, 255, 0xA5A5A5A55A5A5A5A),
            ],
        },
        {
            "reads": [255, 200, 130, 129, 128, 0, 1, 2, 3, 4],
            "writes": [],
        },
    ]

    storage: dict[int, int] = {}
    for cyc, step in enumerate(seq):
        if cyc > 0:
            tb.next()  # --- advance to next cycle ---

        reads = list(step["reads"])
        writes = list(step["writes"])
        drive_cycle(reads, writes)

        for _, waddr, wdata in writes:
            wa = int(waddr)
            if const_count <= wa < ptag_count:
                storage[wa] = int(wdata) & mask64

        for lane in range(nr):
            exp = read_expected(reads[lane], storage)
            tb.expect(
                f"rdata{lane}", exp, msg=f"regfile mismatch cycle={cyc} lane={lane}"
            )

    tb.finish()


if __name__ == "__main__":
    print(compile_cycle_aware(build, name="tb_regfile_top", eager=True).emit_mlir())

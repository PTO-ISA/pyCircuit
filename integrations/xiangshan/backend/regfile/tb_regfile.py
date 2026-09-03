"""Testbench for RegFile — MLIR smoke (L1) + functional directed (L2).

L2 tests: write-after-read consistency, r0 hard-wired zero, multi-port
conflict (last writer wins within same cycle).

Uses tiny config: 8 entries, 2 read, 2 write, 16-bit data, 3-bit addr.
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware, testbench

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from backend.regfile.regfile import build_regfile  # noqa: E402

N_ENT = 8
N_RD = 2
N_WR = 2
D_W = 16
A_W = 3
MASK = (1 << D_W) - 1


@testbench
def tb_regfile_functional(t: Tb) -> None:
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=2, cycles_deasserted=1)
    tb.timeout(40)

    def _zero_inputs():
        for i in range(N_RD):
            tb.drive(f"rd_addr_{i}", 0)
        for i in range(N_WR):
            tb.drive(f"wr_en_{i}", 0)
            tb.drive(f"wr_addr_{i}", 0)
            tb.drive(f"wr_data_{i}", 0)

    # ── Cycle 0: all reads should return 0 after reset ───────────
    _zero_inputs()
    tb.drive("rd_addr_0", 1)
    tb.drive("rd_addr_1", 2)
    tb.expect("rd_data_0", 0, msg="C0: r1 init=0")
    tb.expect("rd_data_1", 0, msg="C0: r2 init=0")

    # ── Cycle 0 cont: write r1=0xCAFE, r2=0xBEEF (synchronous) ──
    tb.drive("wr_en_0", 1)
    tb.drive("wr_addr_0", 1)
    tb.drive("wr_data_0", 0xCAFE)
    tb.drive("wr_en_1", 1)
    tb.drive("wr_addr_1", 2)
    tb.drive("wr_data_1", 0xBEEF)

    # ── Cycle 1: read back written values ─────────────────────────
    tb.next()
    _zero_inputs()
    tb.drive("rd_addr_0", 1)
    tb.drive("rd_addr_1", 2)
    tb.expect("rd_data_0", 0xCAFE, msg="C1: r1 readback")
    tb.expect("rd_data_1", 0xBEEF, msg="C1: r2 readback")

    # ── Cycle 1: write r0=0x1234 (should be ignored) ─────────────
    tb.drive("wr_en_0", 1)
    tb.drive("wr_addr_0", 0)
    tb.drive("wr_data_0", 0x1234)

    # ── Cycle 2: r0 must still read 0 ────────────────────────────
    tb.next()
    _zero_inputs()
    tb.drive("rd_addr_0", 0)
    tb.drive("rd_addr_1", 1)
    tb.expect("rd_data_0", 0, msg="C2: r0 hard-zero after write attempt")
    tb.expect("rd_data_1", 0xCAFE, msg="C2: r1 unchanged")

    # ── Cycle 2: overwrite r1 via port 0, write r3 via port 1 ────
    tb.drive("wr_en_0", 1)
    tb.drive("wr_addr_0", 1)
    tb.drive("wr_data_0", 0x1111)
    tb.drive("wr_en_1", 1)
    tb.drive("wr_addr_1", 3)
    tb.drive("wr_data_1", 0x3333)

    # ── Cycle 3: verify ──────────────────────────────────────────
    tb.next()
    _zero_inputs()
    tb.drive("rd_addr_0", 1)
    tb.drive("rd_addr_1", 3)
    tb.expect("rd_data_0", 0x1111, msg="C3: r1 overwritten")
    tb.expect("rd_data_1", 0x3333, msg="C3: r3 written")

    # ── Cycle 3: both ports write same register (port 1 last) ────
    tb.drive("wr_en_0", 1)
    tb.drive("wr_addr_0", 4)
    tb.drive("wr_data_0", 0xAAAA)
    tb.drive("wr_en_1", 1)
    tb.drive("wr_addr_1", 4)
    tb.drive("wr_data_1", 0xBBBB)

    # ── Cycle 4: read r4 — last writer (port 1) wins ─────────────
    tb.next()
    _zero_inputs()
    tb.drive("rd_addr_0", 4)
    # Both ports write in same cycle; in the RTL, port 1 is applied after
    # port 0, so port 1's value should win.
    tb.expect("rd_data_0", 0xBBBB, msg="C4: r4 multi-write: port1 wins")

    # ── Cycle 4: write with en=0 should not update ───────────────
    tb.drive("wr_en_0", 0)
    tb.drive("wr_addr_0", 5)
    tb.drive("wr_data_0", 0xDEAD)

    # ── Cycle 5: r5 still 0 ──────────────────────────────────────
    tb.next()
    _zero_inputs()
    tb.drive("rd_addr_0", 5)
    tb.expect("rd_data_0", 0, msg="C5: r5 not written (en=0)")

    tb.finish()


# ── L1: MLIR smoke ──────────────────────────────────────────────


@pytest.mark.smoke
def test_regfile_emit_mlir():
    mlir = compile_cycle_aware(
        build_regfile,
        name="rf",
        eager=True,
        num_entries=N_ENT,
        num_read=N_RD,
        num_write=N_WR,
        data_width=D_W,
        addr_width=A_W,
    ).emit_mlir()
    assert "func.func" in mlir
    assert "rd_data_0" in mlir


@pytest.mark.regcount
def test_regfile_has_entry_regs():
    import re

    mlir = compile_cycle_aware(
        build_regfile,
        name="rf_rc",
        eager=True,
        num_entries=N_ENT,
        num_read=N_RD,
        num_write=N_WR,
        data_width=D_W,
        addr_width=A_W,
    ).emit_mlir()
    n = len(re.findall(r"pyc\.reg", mlir))
    assert n >= N_ENT, f"RegFile({N_ENT}) must have ≥{N_ENT} regs; got {n}"


if __name__ == "__main__":
    test_regfile_emit_mlir()
    print("PASS: test_regfile_emit_mlir")

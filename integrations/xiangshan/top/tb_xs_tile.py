"""Testbench for XSTile integration module."""

from __future__ import annotations

import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from pycircuit import compile_cycle_aware

from top.xs_tile import build_xs_tile


def test_xs_tile_small_emit_mlir():
    """Smoke test: XSTile compiles to MLIR with small config."""
    circuit = compile_cycle_aware(
        build_xs_tile,
        name="xs_tile",
        eager=True,
        decode_width=2,
        commit_width=2,
        num_wb=2,
        num_load=1,
        num_store=1,
        data_width=16,
        pc_width=16,
        ptag_w=4,
        rob_idx_w=4,
        fu_type_w=3,
        block_bits=128,
        hart_id_w=4,
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir
    assert "xs_tile" in mlir
    assert "hart_id" in mlir
    assert "ds_req_valid" in mlir
    assert "redirect_valid" in mlir
    assert "dc_refill_valid" in mlir
    print(f"XSTile (small) MLIR: {len(mlir)} chars, compilation OK")


def test_xs_tile_medium_emit_mlir():
    """XSTile with moderate config."""
    circuit = compile_cycle_aware(
        build_xs_tile,
        name="xs_tile_med",
        eager=True,
        decode_width=4,
        commit_width=4,
        num_wb=4,
        num_load=2,
        num_store=1,
        data_width=64,
        pc_width=39,
        ptag_w=8,
        rob_idx_w=9,
        fu_type_w=3,
        block_bits=512,
        hart_id_w=4,
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir
    assert "interrupt_pending" in mlir
    print(f"XSTile (medium) MLIR: {len(mlir)} chars, compilation OK")


if __name__ == "__main__":
    test_xs_tile_small_emit_mlir()
    print("PASS: test_xs_tile_small_emit_mlir")
    test_xs_tile_medium_emit_mlir()
    print("PASS: test_xs_tile_medium_emit_mlir")

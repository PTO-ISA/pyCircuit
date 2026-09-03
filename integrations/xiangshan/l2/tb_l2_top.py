"""Testbench for L2 Top cache interface shell."""

from __future__ import annotations

import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from pycircuit import compile_cycle_aware

from l2.l2_top import build_l2_top


def test_l2_top_small_emit_mlir():
    """Smoke test: L2 Top compiles to MLIR with small config."""
    circuit = compile_cycle_aware(
        build_l2_top,
        name="l2_top",
        eager=True,
        addr_width=16,
        data_width=16,
        block_bits=128,
        queue_size=4,
        tag_w=8,
        idx_w=4,
        num_ways=2,
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir
    assert "l2_top" in mlir
    assert "ic_req_valid" in mlir
    assert "dc_req_valid" in mlir
    assert "ds_req_valid" in mlir
    assert "ic_grant_valid" in mlir
    print(f"L2 Top (small) MLIR: {len(mlir)} chars, compilation OK")


def test_l2_top_default_emit_mlir():
    """L2 Top with larger queue."""
    circuit = compile_cycle_aware(
        build_l2_top,
        name="l2_top_med",
        eager=True,
        addr_width=39,
        data_width=64,
        block_bits=512,
        queue_size=8,
        tag_w=20,
        idx_w=10,
        num_ways=4,
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir
    assert "l2_busy" in mlir
    print(f"L2 Top MLIR: {len(mlir)} chars, compilation OK")


if __name__ == "__main__":
    test_l2_top_small_emit_mlir()
    print("PASS: test_l2_top_small_emit_mlir")
    test_l2_top_default_emit_mlir()
    print("PASS: test_l2_top_default_emit_mlir")

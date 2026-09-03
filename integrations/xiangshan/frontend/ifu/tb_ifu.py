"""Testbench for IFU module."""

from __future__ import annotations

import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from pycircuit import compile_cycle_aware

from frontend.ifu.ifu import build_ifu, FETCH_WIDTH, CACHE_DATA_WIDTH
from top.parameters import PC_WIDTH


def test_ifu_small_emit_mlir():
    """Small config (4 slots) for faster compilation."""
    circuit = compile_cycle_aware(
        build_ifu,
        name="ifu_small",
        eager=True,
        fetch_width=4,
        pc_width=PC_WIDTH,
        cache_data_width=64,
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir
    assert "@ifu_small" in mlir
    print(f"IFU (small) MLIR: {len(mlir)} chars, compilation OK")


def test_ifu_emit_mlir():
    """Smoke test: full-width IFU compiles to MLIR."""
    circuit = compile_cycle_aware(
        build_ifu,
        name="ifu",
        eager=True,
        fetch_width=FETCH_WIDTH,
        pc_width=PC_WIDTH,
        cache_data_width=CACHE_DATA_WIDTH,
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir
    assert "@ifu" in mlir
    print(f"IFU MLIR: {len(mlir)} chars, compilation OK")


if __name__ == "__main__":
    test_ifu_small_emit_mlir()
    print("PASS: test_ifu_small_emit_mlir")
    test_ifu_emit_mlir()
    print("PASS: test_ifu_emit_mlir")

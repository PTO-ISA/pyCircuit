"""Testbench for CtrlBlock module."""

from __future__ import annotations

import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from pycircuit import compile_cycle_aware

from backend.ctrlblock.ctrlblock import build_ctrlblock


def test_ctrlblock_small_emit_mlir():
    """Smoke test: CtrlBlock compiles to MLIR with small config."""
    circuit = compile_cycle_aware(
        build_ctrlblock,
        name="ctrlblock",
        eager=True,
        decode_width=2,
        commit_width=2,
        ptag_w=4,
        pc_width=16,
        rob_idx_w=4,
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir
    assert "@ctrlblock" in mlir
    assert "redirect_valid" in mlir
    assert "stall_to_frontend" in mlir
    print(f"CtrlBlock (small) MLIR: {len(mlir)} chars, compilation OK")


def test_ctrlblock_emit_mlir():
    """Smoke test: CtrlBlock compiles to MLIR at moderate width."""
    circuit = compile_cycle_aware(
        build_ctrlblock,
        name="ctrlblock_med",
        eager=True,
        decode_width=4,
        commit_width=4,
        ptag_w=8,
        pc_width=39,
        rob_idx_w=9,
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir
    assert "redirect" in mlir
    print(f"CtrlBlock MLIR: {len(mlir)} chars, compilation OK")


if __name__ == "__main__":
    test_ctrlblock_small_emit_mlir()
    print("PASS: test_ctrlblock_small_emit_mlir")
    test_ctrlblock_emit_mlir()
    print("PASS: test_ctrlblock_emit_mlir")

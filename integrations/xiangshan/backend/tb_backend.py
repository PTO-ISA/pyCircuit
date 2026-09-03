"""Testbench for Backend top-level module."""

from __future__ import annotations

import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from pycircuit import compile_cycle_aware

from backend.backend import build_backend


def test_backend_small_emit_mlir():
    """Smoke test: Backend compiles to MLIR with small config."""
    circuit = compile_cycle_aware(
        build_backend,
        name="backend",
        eager=True,
        decode_width=2,
        commit_width=2,
        num_wb=2,
        num_int_exu=1,
        num_fp_exu=1,
        ptag_w=4,
        data_width=16,
        pc_width=16,
        rob_idx_w=4,
        fu_type_w=3,
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir
    assert "@backend" in mlir
    assert "redirect_valid" in mlir
    assert "stall_to_frontend" in mlir
    print(f"Backend (small) MLIR: {len(mlir)} chars, compilation OK")


def test_backend_emit_mlir():
    """Smoke test: Backend compiles to MLIR at moderate width."""
    circuit = compile_cycle_aware(
        build_backend,
        name="backend_med",
        eager=True,
        decode_width=4,
        commit_width=4,
        num_wb=4,
        num_int_exu=2,
        num_fp_exu=1,
        ptag_w=8,
        data_width=64,
        pc_width=39,
        rob_idx_w=9,
        fu_type_w=3,
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir
    assert "redirect" in mlir
    assert "mem_dp_valid" in mlir
    print(f"Backend MLIR: {len(mlir)} chars, compilation OK")


if __name__ == "__main__":
    test_backend_small_emit_mlir()
    print("PASS: test_backend_small_emit_mlir")
    test_backend_emit_mlir()
    print("PASS: test_backend_emit_mlir")

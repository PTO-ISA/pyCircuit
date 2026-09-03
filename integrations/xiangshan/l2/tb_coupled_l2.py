"""Testbench for CoupledL2 module."""

from __future__ import annotations

import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from pycircuit import compile_cycle_aware

from l2.coupled_l2 import (
    L2_MSHR_COUNT,
    L2_REQ_BUF_ENTRIES,
    L2_SETS,
    L2_WAYS,
    build_coupled_l2,
)

from top.parameters import CACHE_LINE_SIZE, PADDR_BITS_MAX


def test_coupled_l2_small_emit_mlir():
    """Smaller config (4 sets, 2 ways) for fast compilation smoke test."""
    circuit = compile_cycle_aware(
        build_coupled_l2,
        name="coupled_l2_small",
        eager=True,
        sets=4,
        ways=2,
        addr_width=32,
        data_width=64,
        mshr_count=2,
        req_buf_entries=2,
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir
    assert "@coupled_l2_small" in mlir
    assert "pyc.reg" in mlir or "pyc.state" in mlir or "hw." in mlir
    print(f"CoupledL2 (small) MLIR: {len(mlir)} chars, compilation OK")


def test_coupled_l2_emit_mlir():
    """Full-size CoupledL2 compiles to MLIR successfully."""
    circuit = compile_cycle_aware(
        build_coupled_l2,
        name="coupled_l2",
        eager=True,
        sets=L2_SETS,
        ways=L2_WAYS,
        addr_width=PADDR_BITS_MAX,
        data_width=CACHE_LINE_SIZE,
        mshr_count=L2_MSHR_COUNT,
        req_buf_entries=L2_REQ_BUF_ENTRIES,
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir
    assert "@coupled_l2" in mlir
    assert "pyc.reg" in mlir or "pyc.state" in mlir or "hw." in mlir
    print(f"CoupledL2 MLIR: {len(mlir)} chars, compilation OK")


if __name__ == "__main__":
    test_coupled_l2_small_emit_mlir()
    print("PASS: test_coupled_l2_small_emit_mlir")
    test_coupled_l2_emit_mlir()
    print("PASS: test_coupled_l2_emit_mlir")

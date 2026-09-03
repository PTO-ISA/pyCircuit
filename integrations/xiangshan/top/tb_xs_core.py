"""Testbench for XSCore integration module."""

from __future__ import annotations

import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from pycircuit import compile_cycle_aware

from top.xs_core import build_xs_core


def test_xs_core_small_emit_mlir():
    """Smoke test: XSCore compiles to MLIR with small config."""
    circuit = compile_cycle_aware(
        build_xs_core,
        name="xs_core",
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
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir
    assert "xs_core" in mlir
    assert "redirect_valid" in mlir
    assert "l2_icache_miss" in mlir
    assert "debug_pc" in mlir
    print(f"XSCore (small) MLIR: {len(mlir)} chars, compilation OK")


def test_xs_core_medium_emit_mlir():
    """XSCore with moderate config."""
    circuit = compile_cycle_aware(
        build_xs_core,
        name="xs_core_med",
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
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir
    assert "mem_dp_valid" in mlir
    assert "interrupt_pending" in mlir
    print(f"XSCore (medium) MLIR: {len(mlir)} chars, compilation OK")


if __name__ == "__main__":
    test_xs_core_small_emit_mlir()
    print("PASS: test_xs_core_small_emit_mlir")
    test_xs_core_medium_emit_mlir()
    print("PASS: test_xs_core_medium_emit_mlir")

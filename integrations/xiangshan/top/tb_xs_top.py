"""Testbench for XSTop SoC wrapper module."""

from __future__ import annotations

import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from pycircuit import compile_cycle_aware

from top.xs_top import build_xs_top


def test_xs_top_small_emit_mlir():
    """Smoke test: XSTop compiles to MLIR with small dual-core config."""
    circuit = compile_cycle_aware(
        build_xs_top,
        name="xs_top",
        eager=True,
        num_cores=2,
        data_width=16,
        addr_width=16,
        block_bits=128,
        hart_id_w=4,
        axi_id_w=4,
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir
    assert "xs_top" in mlir
    assert "axi_ar_valid" in mlir
    assert "tile0_ds_resp_valid" in mlir
    assert "tile1_ds_resp_valid" in mlir
    assert "tile0_hart_id" in mlir
    assert "debug_resp_valid" in mlir
    print(f"XSTop (2-core small) MLIR: {len(mlir)} chars, compilation OK")


def test_xs_top_single_core_emit_mlir():
    """XSTop with single core."""
    circuit = compile_cycle_aware(
        build_xs_top,
        name="xs_top_1c",
        eager=True,
        num_cores=1,
        data_width=16,
        addr_width=16,
        block_bits=128,
        hart_id_w=4,
        axi_id_w=4,
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir
    assert "tile0_meip" in mlir
    assert "axi_ar_addr" in mlir
    print(f"XSTop (1-core) MLIR: {len(mlir)} chars, compilation OK")


if __name__ == "__main__":
    test_xs_top_small_emit_mlir()
    print("PASS: test_xs_top_small_emit_mlir")
    test_xs_top_single_core_emit_mlir()
    print("PASS: test_xs_top_single_core_emit_mlir")

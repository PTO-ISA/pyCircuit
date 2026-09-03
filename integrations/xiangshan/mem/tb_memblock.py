"""Testbench for MemBlock top-level — MLIR emission smoke tests."""

from __future__ import annotations

import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from pycircuit import compile_cycle_aware

from mem.memblock import build_memblock


def test_memblock_small_emit_mlir():
    """Smoke test: single load/store memblock compiles to MLIR."""
    circuit = compile_cycle_aware(
        build_memblock,
        name="memblock_small",
        eager=True,
        num_load=1,
        num_store=1,
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir or "hw." in mlir, "expected MLIR output"
    assert "memblock" in mlir, "expected module name in MLIR"

    expected_ports = [
        "ld0_issue_valid",
        "st0_issue_valid",
        "ld0_wb_valid",
        "st0_wb_valid",
        "dcache_req_valid",
        "tlb_req_valid",
        "flush",
    ]
    for port in expected_ports:
        assert port in mlir, f"expected port '{port}' in MLIR"

    print(f"MemBlock (small) MLIR: {len(mlir)} chars, compilation OK")


def test_memblock_default_emit_mlir():
    """Smoke test: full 3-load/2-store memblock compiles to MLIR."""
    circuit = compile_cycle_aware(
        build_memblock,
        name="memblock",
        eager=True,
        num_load=3,
        num_store=2,
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir or "hw." in mlir, "expected MLIR output"

    for i in range(3):
        assert f"ld{i}_issue_valid" in mlir, f"expected ld{i} ports in MLIR"
    for i in range(2):
        assert f"st{i}_issue_valid" in mlir, f"expected st{i} ports in MLIR"

    print(f"MemBlock MLIR: {len(mlir)} chars, compilation OK")


if __name__ == "__main__":
    test_memblock_small_emit_mlir()
    print("PASS: test_memblock_small_emit_mlir")
    test_memblock_default_emit_mlir()
    print("PASS: test_memblock_default_emit_mlir")

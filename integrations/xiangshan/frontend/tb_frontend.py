"""Testbench for Frontend top-level module."""

from __future__ import annotations

import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent
_SCRIPT_DIR = str(Path(__file__).resolve().parent)
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))
# Remove script directory so that frontend.py doesn't shadow the frontend/ package
if _SCRIPT_DIR in sys.path:
    sys.path.remove(_SCRIPT_DIR)

from pycircuit import compile_cycle_aware

from frontend.frontend import build_frontend


def test_frontend_small_emit_mlir():
    """Smoke test: Frontend compiles to MLIR with small config."""
    circuit = compile_cycle_aware(
        build_frontend,
        name="frontend",
        eager=True,
        decode_width=2,
        pc_width=16,
        fetch_width=4,
        inst_width=32,
        block_bits=128,
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir
    assert "@frontend" in mlir
    print(f"Frontend (small) MLIR: {len(mlir)} chars, compilation OK")


def test_frontend_medium_emit_mlir():
    """Frontend with larger config."""
    circuit = compile_cycle_aware(
        build_frontend,
        name="frontend_medium",
        eager=True,
        decode_width=4,
        pc_width=39,
        fetch_width=8,
        inst_width=32,
        block_bits=512,
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir
    print(f"Frontend (medium) MLIR: {len(mlir)} chars, compilation OK")


if __name__ == "__main__":
    test_frontend_small_emit_mlir()
    print("PASS: test_frontend_small_emit_mlir")
    test_frontend_medium_emit_mlir()
    print("PASS: test_frontend_medium_emit_mlir")

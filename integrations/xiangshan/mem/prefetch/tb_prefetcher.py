"""Testbench for Prefetcher — MLIR emission smoke tests."""

from __future__ import annotations

import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from pycircuit import compile_cycle_aware

from mem.prefetch.prefetcher import build_prefetcher


def test_prefetcher_small_emit_mlir():
    """Smoke test: small prefetcher compiles to MLIR."""
    circuit = compile_cycle_aware(
        build_prefetcher,
        name="prefetcher_small",
        eager=True,
        table_size=4,
        addr_width=36,
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir or "hw." in mlir, "expected MLIR output"
    assert "prefetcher" in mlir, "expected module name in MLIR"

    expected_ports = ["train_valid", "train_pc", "train_addr", "pf_valid", "pf_addr"]
    for port in expected_ports:
        assert port in mlir, f"expected port '{port}' in MLIR"

    print(f"Prefetcher (small) MLIR: {len(mlir)} chars, compilation OK")


def test_prefetcher_default_emit_mlir():
    """Smoke test: default-size prefetcher compiles to MLIR."""
    circuit = compile_cycle_aware(
        build_prefetcher,
        name="prefetcher",
        eager=True,
        table_size=16,
        addr_width=36,
    )
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir or "hw." in mlir, "expected MLIR output"
    print(f"Prefetcher MLIR: {len(mlir)} chars, compilation OK")


if __name__ == "__main__":
    test_prefetcher_small_emit_mlir()
    print("PASS: test_prefetcher_small_emit_mlir")
    test_prefetcher_default_emit_mlir()
    print("PASS: test_prefetcher_default_emit_mlir")

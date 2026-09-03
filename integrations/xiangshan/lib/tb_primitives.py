"""Testbench for lib/primitives.py combinational primitives."""

from __future__ import annotations

from pycircuit import compile_cycle_aware

from lib.primitives import (
    build_leading_zeros,
    build_mux1h,
    build_popcount,
    build_priority_enc,
)


def _smoke_emit(builder, name: str, **kwargs) -> str:
    """Compile a primitive builder and return MLIR text."""
    circuit = compile_cycle_aware(builder, name=name, eager=True, **kwargs)
    mlir = circuit.emit_mlir()
    assert "func.func" in mlir, f"{name}: emit_mlir did not produce func.func"
    assert f"@{name}" in mlir, f"{name}: function name not found in MLIR"
    return mlir


def test_mux1h_emit():
    _smoke_emit(build_mux1h, "mux1h", n=4, width=8)


def test_popcount_emit():
    _smoke_emit(build_popcount, "popcount", n=8)


def test_priority_enc_emit():
    _smoke_emit(build_priority_enc, "priority_enc", n=8)


def test_leading_zeros_emit():
    _smoke_emit(build_leading_zeros, "leading_zeros", n=8)


if __name__ == "__main__":
    for fn in [
        test_mux1h_emit,
        test_popcount_emit,
        test_priority_enc_emit,
        test_leading_zeros_emit,
    ]:
        fn()
        print(f"PASS: {fn.__name__}")

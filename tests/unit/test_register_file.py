from __future__ import annotations

import re

import pytest
from pycircuit import build_cycle_aware

from designs.blocks.RegisterFile.regfile import build

pytestmark = pytest.mark.unit


def test_register_file_keeps_cycle_zero_state_snapshot() -> None:
    mlir = build_cycle_aware(build, name="regfile").emit_mlir()

    balance_registers = re.findall(r"\b_v6_bal_[A-Za-z0-9_]*\b", mlir)
    register_ops = re.findall(r"\bpyc\.reg\b", mlir)

    assert balance_registers == []
    assert len(register_ops) == 256

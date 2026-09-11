from __future__ import annotations

import importlib.util
import re
from pathlib import Path

import pytest
from pycircuit import build_cycle_aware

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]
DESIGN = ROOT / "tests/integration/pycircuit/fixtures/regfile/regfile.py"


def _build_function():
    spec = importlib.util.spec_from_file_location("regfile_unit_fixture", DESIGN)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load register-file fixture")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.build


def test_register_file_keeps_cycle_zero_state_snapshot() -> None:
    mlir = build_cycle_aware(_build_function(), name="regfile").emit_mlir()

    balance_registers = re.findall(r"\b_v6_bal_[A-Za-z0-9_]*\b", mlir)
    register_ops = re.findall(r"\bpyc\.reg\b", mlir)

    assert balance_registers == []
    assert len(register_ops) == 256

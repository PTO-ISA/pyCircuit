from __future__ import annotations

import importlib.util
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]
DESIGN = ROOT / "examples/pycircuit/dodgeball_game/lab_final_top.py"


def test_dodgeball_cycle_aware_example_emits_canonical_pyc() -> None:
    spec = importlib.util.spec_from_file_location("dodgeball_system_gate", DESIGN)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load dodgeball example")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)

    mlir = module.compile_cycle_aware(
        module.build,
        name="dodgeball_game",
        MAIN_CLK_BIT=4,
    ).emit_mlir()

    assert mlir.count("pyc.reg") == 14
    assert "_v6_bal_" not in mlir
    for port in (
        "VGA_HS_O",
        "VGA_VS_O",
        "VGA_R",
        "VGA_G",
        "VGA_B",
        "dbg_state",
        "dbg_player_x",
    ):
        assert port in mlir

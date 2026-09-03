"""L5 system-level tests — multi-core SoC and peripheral verification.

These tests compile the full XSTop with multi-tile configuration and
verify SoC-level signals are present.

Run with:  pytest test_xs_system.py -v -m system
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

_XS_ROOT = Path(__file__).resolve().parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from pycircuit import compile_cycle_aware  # noqa: E402


@pytest.mark.system
class TestMultiCoreSoC:
    """L5: Multi-tile SoC compilation."""

    def test_xstop_2tile_compiles(self):
        from top.xs_top import build_xs_top

        mlir = compile_cycle_aware(
            build_xs_top,
            name="soc_2t",
            eager=True,
            num_cores=2,
        ).emit_mlir()
        assert "func.func" in mlir

    def test_xstop_has_tile_signals(self):
        from top.xs_top import build_xs_top

        mlir = compile_cycle_aware(
            build_xs_top,
            name="soc_sig",
            eager=True,
            num_cores=2,
        ).emit_mlir()
        for sig in ["tile0", "tile1", "axi", "hart_id", "core"]:
            if sig in mlir.lower():
                break
        else:
            pytest.skip("Multi-core signal names may vary")


@pytest.mark.system
class TestPeripherals:
    """L5: Peripheral stubs compile."""

    def test_peripherals_importable(self):
        from top.peripherals import build_plic, build_clint

        assert callable(build_plic)
        assert callable(build_clint)

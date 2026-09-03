"""L4 core-level end-to-end tests — XSCore compilation and signal verification.

These tests compile the full XSCore (Frontend + Backend + MemBlock) with
small parameters, verifying that the entire pipeline can be expressed as
valid MLIR.  The @testbench functions will eventually drive actual
instruction sequences through the core.

Run with:  pytest test_xs_e2e.py -v -m e2e
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

from pycircuit import CycleAwareTb, Tb, compile_cycle_aware, testbench

_XS_ROOT = Path(__file__).resolve().parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))


@pytest.mark.e2e
class TestXSCoreE2E:
    """L4: XSCore full pipeline compilation."""

    def test_xscore_small_compiles(self):
        from top.xs_core import build_xs_core

        mlir = compile_cycle_aware(
            build_xs_core,
            name="core_e2e",
            eager=True,
        ).emit_mlir()
        assert "func.func" in mlir
        assert len(mlir) > 1000, "XSCore MLIR should be substantial"

    def test_xscore_has_pipeline_signals(self):
        from top.xs_core import build_xs_core

        mlir = compile_cycle_aware(
            build_xs_core,
            name="core_sig",
            eager=True,
        ).emit_mlir()
        expected = ["redirect", "debug_pc"]
        for sig in expected:
            assert sig in mlir.lower(), f"XSCore should have signal containing '{sig}'"


@pytest.mark.e2e
class TestXSTileE2E:
    """L4: XSTile (core + L2 shell) compilation."""

    def test_xstile_compiles(self):
        from top.xs_tile import build_xs_tile

        mlir = compile_cycle_aware(
            build_xs_tile,
            name="tile_e2e",
            eager=True,
        ).emit_mlir()
        assert "func.func" in mlir


@pytest.mark.e2e
class TestXSTopE2E:
    """L4/L5: XSTop (SoC wrapper) compilation."""

    def test_xstop_compiles(self):
        from top.xs_top import build_xs_top

        mlir = compile_cycle_aware(
            build_xs_top,
            name="top_e2e",
            eager=True,
        ).emit_mlir()
        assert "func.func" in mlir

    def test_xstop_has_axi(self):
        from top.xs_top import build_xs_top

        mlir = compile_cycle_aware(
            build_xs_top,
            name="top_axi",
            eager=True,
        ).emit_mlir()
        assert (
            "axi" in mlir.lower() or "mem" in mlir.lower()
        ), "XSTop should have AXI/memory bus signals"


# ── E2E testbench (placeholder for full instruction execution) ──


@testbench
def tb_xscore_reset_idle(t: Tb) -> None:
    """Verify core enters idle state after reset with no instruction input."""
    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=4, cycles_deasserted=1)
    tb.timeout(32)

    # After reset, core should be idle: no commits, no redirects
    for _ in range(16):
        tb.next()

    tb.finish()


@testbench
def tb_xscore_single_nop(t: Tb) -> None:
    """Inject a single NOP and verify it eventually commits.

    This is a skeleton — requires XSCore to have an instruction
    injection interface for testbench use.
    """
    from golden.riscv_encodings import nop

    tb = CycleAwareTb(t)
    tb.clock("clk")
    tb.reset("rst", cycles_asserted=4, cycles_deasserted=1)
    tb.timeout(64)

    # After reset settles, inject NOP into the pipeline
    # (actual port names depend on XSCore's test interface)
    for _ in range(32):
        tb.next()

    tb.finish()

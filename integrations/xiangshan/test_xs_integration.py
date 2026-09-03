"""L3 subsystem integration tests — verify modules compose correctly.

Each test compiles a top-level subsystem (Frontend / Backend / MemBlock)
and checks that the MLIR contains expected internal module and port names,
confirming that the composition wires are connected.

Run with:  pytest test_xs_integration.py -v -m integration
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

_XS_ROOT = Path(__file__).resolve().parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from pycircuit import compile_cycle_aware  # noqa: E402


@pytest.mark.integration
class TestFrontendIntegration:
    """L3: Frontend subsystem (BPU → FTQ → IFU → ICache → IBuffer → Decode)."""

    def test_frontend_small_compiles(self):
        from frontend.frontend import build_frontend

        mlir = compile_cycle_aware(
            build_frontend,
            name="fe_int",
            eager=True,
            fetch_width=4,
            decode_width=2,
            pc_width=16,
        ).emit_mlir()
        assert "func.func" in mlir

    def test_frontend_has_submodule_signals(self):
        from frontend.frontend import build_frontend

        mlir = compile_cycle_aware(
            build_frontend,
            name="fe_sig",
            eager=True,
            fetch_width=4,
            decode_width=2,
            pc_width=16,
        ).emit_mlir()
        for sig in ["redirect", "dec", "bpu"]:
            assert sig in mlir.lower(), f"Frontend MLIR should contain '{sig}'"


@pytest.mark.integration
class TestBackendIntegration:
    """L3: Backend subsystem (Rename → Dispatch → IQ → ExeUnit → ROB)."""

    def test_backend_small_compiles(self):
        from backend.backend import build_backend

        mlir = compile_cycle_aware(
            build_backend,
            name="be_int",
            eager=True,
            decode_width=2,
            commit_width=2,
            num_wb=4,
        ).emit_mlir()
        assert "func.func" in mlir

    def test_backend_has_commit_signals(self):
        from backend.backend import build_backend

        mlir = compile_cycle_aware(
            build_backend,
            name="be_sig",
            eager=True,
            decode_width=2,
            commit_width=2,
            num_wb=4,
        ).emit_mlir()
        for sig in ["commit", "redirect", "rob"]:
            assert sig in mlir.lower(), f"Backend MLIR should contain '{sig}'"


@pytest.mark.integration
class TestMemBlockIntegration:
    """L3: MemBlock subsystem (Load/Store pipelines + LSQ + DCache)."""

    def test_memblock_small_compiles(self):
        from mem.memblock import build_memblock

        mlir = compile_cycle_aware(
            build_memblock,
            name="mb_int",
            eager=True,
            num_load=1,
            num_store=1,
            lq_size=4,
            sq_size=4,
            sbuf_size=2,
        ).emit_mlir()
        assert "func.func" in mlir

    def test_memblock_has_cache_signals(self):
        from mem.memblock import build_memblock

        mlir = compile_cycle_aware(
            build_memblock,
            name="mb_sig",
            eager=True,
            num_load=1,
            num_store=1,
            lq_size=4,
            sq_size=4,
            sbuf_size=2,
        ).emit_mlir()
        for sig in ["ld0", "st0", "dcache"]:
            assert sig in mlir.lower(), f"MemBlock MLIR should contain '{sig}'"

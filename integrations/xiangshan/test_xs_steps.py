"""XiangShan-pyc ten-step verification pytest suite.

Run with: pytest test_xs_steps.py -v
Markers: step1..step10, phase0..phase5, frontend, backend, memblock
"""

from __future__ import annotations

import importlib
import sys
from pathlib import Path

import pytest

XS_ROOT = Path(__file__).resolve().parent
if str(XS_ROOT) not in sys.path:
    sys.path.insert(0, str(XS_ROOT))


# ---------------------------------------------------------------------------
# Phase 0: Infrastructure
# ---------------------------------------------------------------------------


@pytest.mark.phase0
class TestPhase0:
    def test_parameters_importable(self):
        mod = importlib.import_module("top.parameters")
        assert hasattr(mod, "XLEN")
        assert mod.XLEN == 64

    def test_primitives_importable(self):
        mod = importlib.import_module("lib.primitives")
        assert callable(getattr(mod, "build_mux1h", None))
        assert callable(getattr(mod, "build_popcount", None))
        assert callable(getattr(mod, "build_priority_enc", None))

    def test_tilelink_importable(self):
        mod = importlib.import_module("lib.tilelink")
        assert hasattr(mod, "TL_A_OPCODE_GET")

    def test_axi_importable(self):
        mod = importlib.import_module("lib.axi")
        assert hasattr(mod, "axi4_aw_ports")


# ---------------------------------------------------------------------------
# Phase 1: Frontend
# ---------------------------------------------------------------------------


@pytest.mark.phase1
@pytest.mark.frontend
class TestFrontend:
    @pytest.mark.parametrize(
        "module_name",
        [
            "frontend.ibuffer.ibuffer",
            "frontend.ftq.ftq",
            "frontend.bpu.bpu",
            "frontend.bpu.ubtb",
            "frontend.bpu.tage",
            "frontend.bpu.sc",
            "frontend.bpu.ittage",
            "frontend.bpu.ras",
            "frontend.icache.icache",
            "frontend.ifu.ifu",
            "frontend.decode.decode",
            "frontend.frontend",
        ],
    )
    def test_module_importable(self, module_name):
        mod = importlib.import_module(module_name)
        build_fn = None
        for name in dir(mod):
            if name.startswith("build_"):
                build_fn = getattr(mod, name)
                break
        assert build_fn is not None, f"No build_* function found in {module_name}"


# ---------------------------------------------------------------------------
# Phase 2: Backend
# ---------------------------------------------------------------------------


@pytest.mark.phase2
@pytest.mark.backend
class TestBackend:
    @pytest.mark.parametrize(
        "module_name",
        [
            "backend.rename.rename",
            "backend.rob.rob",
            "backend.dispatch.dispatch",
            "backend.issue.issue_queue",
            "backend.regfile.regfile",
            "backend.exu.alu",
            "backend.exu.mul",
            "backend.exu.div",
            "backend.exu.bru",
            "backend.backend",
        ],
    )
    def test_module_importable(self, module_name):
        mod = importlib.import_module(module_name)
        build_fn = None
        for name in dir(mod):
            if name.startswith("build_"):
                build_fn = getattr(mod, name)
                break
        assert build_fn is not None, f"No build_* function found in {module_name}"


# ---------------------------------------------------------------------------
# Phase 3: MemBlock + Cache
# ---------------------------------------------------------------------------


@pytest.mark.phase3
@pytest.mark.memblock
class TestMemBlock:
    @pytest.mark.parametrize(
        "module_name",
        [
            "cache.mmu.tlb",
            "cache.dcache.dcache",
            "mem.pipeline.load_unit",
            "mem.pipeline.store_unit",
            "mem.lsqueue.load_queue",
            "mem.lsqueue.store_queue",
            "mem.sbuffer.sbuffer",
            "mem.memblock",
        ],
    )
    def test_module_importable(self, module_name):
        mod = importlib.import_module(module_name)
        build_fn = None
        for name in dir(mod):
            if name.startswith("build_"):
                build_fn = getattr(mod, name)
                break
        assert build_fn is not None, f"No build_* function found in {module_name}"


# ---------------------------------------------------------------------------
# Phase 4: Core integration
# ---------------------------------------------------------------------------


@pytest.mark.phase4
class TestCoreIntegration:
    @pytest.mark.parametrize(
        "module_name",
        [
            "top.xs_core",
            "top.xs_tile",
            "top.xs_top",
        ],
    )
    def test_module_importable(self, module_name):
        mod = importlib.import_module(module_name)
        build_fn = None
        for name in dir(mod):
            if name.startswith("build_"):
                build_fn = getattr(mod, name)
                break
        assert build_fn is not None, f"No build_* function found in {module_name}"

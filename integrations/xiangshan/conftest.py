"""Pytest configuration for XiangShan-pyc verification suite.

Marker hierarchy maps to verification levels:
  smoke        L1  MLIR compilation
  functional   L2  Directed functional tests (@testbench)
  integration  L3  Subsystem integration
  e2e          L4  Core-level end-to-end
  system       L5  SoC / multi-core
  sva          SVA formal property compilation
  regcount     pyc.reg count regression
  verilog      pycc --emit=verilog gate
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

_XS_ROOT = Path(__file__).resolve().parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))


def pytest_configure(config: pytest.Config) -> None:
    for marker, desc in [
        ("smoke", "L1: MLIR compilation smoke tests"),
        ("functional", "L2: Directed functional tests"),
        ("integration", "L3: Subsystem integration tests"),
        ("e2e", "L4: Core-level end-to-end tests"),
        ("system", "L5: SoC / multi-core system tests"),
        ("sva", "SVA formal property compilation tests"),
        ("regcount", "pyc.reg count regression tests"),
        ("verilog", "pycc --emit=verilog compilation gate"),
        ("phase0", "Phase 0: Infrastructure"),
        ("phase1", "Phase 1: Frontend"),
        ("phase2", "Phase 2: Backend"),
        ("phase3", "Phase 3: MemBlock + Cache"),
        ("phase4", "Phase 4: Core integration"),
        ("frontend", "Frontend subsystem tests"),
        ("backend", "Backend subsystem tests"),
        ("memblock", "MemBlock subsystem tests"),
    ]:
        config.addinivalue_line("markers", f"{marker}: {desc}")


@pytest.fixture
def small_cfg():
    """Minimal parameter set for fast MLIR compilation."""
    return dict(data_width=16, pc_width=16, ptag_w=4, lreg_w=3, addr_width=3)

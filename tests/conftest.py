from __future__ import annotations

import sys
from pathlib import Path

# Make the in-tree frontend package importable without requiring callers to set
# PYTHONPATH, so gates run under a plain ``pytest`` invocation.
_FRONTEND = Path(__file__).resolve().parents[1] / "python" / "pycircuit" / "src"
if _FRONTEND.is_dir():
    p = str(_FRONTEND)
    if p not in sys.path:
        sys.path.insert(0, p)

_AGENTIC_BUILD_PYTHON = (
    Path(__file__).resolve().parents[1]
    / ".pycircuit_out"
    / "acir"
    / "dev-llvm22"
    / "python"
)
if _AGENTIC_BUILD_PYTHON.is_dir():
    p = str(_AGENTIC_BUILD_PYTHON)
    if p not in sys.path:
        sys.path.append(p)

# Also expose this tests/ dir so shared helpers import regardless of pytest's
# import mode.
_TESTS_DIR = str(Path(__file__).resolve().parent)
if _TESTS_DIR not in sys.path:
    sys.path.insert(0, _TESTS_DIR)

_AGENTIC_TESTS_DIR = str(Path(__file__).resolve().parent / "python" / "agentic-circuit")
if _AGENTIC_TESTS_DIR not in sys.path:
    sys.path.insert(0, _AGENTIC_TESTS_DIR)

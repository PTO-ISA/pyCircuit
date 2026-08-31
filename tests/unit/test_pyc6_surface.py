from __future__ import annotations

import importlib.util
from pathlib import Path

import pycircuit
import pycircuit.v6 as pyc6
import pytest

pytestmark = pytest.mark.unit


def test_cycle_aware_frontend_is_the_pyc6_surface() -> None:
    assert pycircuit.CycleAwareSignal is pyc6.CycleAwareSignal
    assert pycircuit.CycleAwareDomain is pyc6.CycleAwareDomain
    assert pycircuit.compile_cycle_aware is pyc6.compile_cycle_aware
    assert not hasattr(pycircuit, "StateSignal")


def test_pyc5_module_is_not_shipped_as_a_compatibility_surface() -> None:
    assert importlib.util.find_spec("pycircuit.v5") is None


def test_runtime_and_trace_identifiers_are_pyc6_only() -> None:
    root = Path(__file__).resolve().parents[2]
    contract_files = (
        root / "CMakeLists.txt",
        root / "runtime/cpp/CMakeLists.txt",
        root / "runtime/cpp/pyc_runtime.cpp",
        root / "runtime/cpp/pyc_trace_bin.hpp",
        root / "compiler/frontend/pycircuit/cli.py",
        root / "compiler/mlir/tools/pycc.cpp",
        root / "flows/tools/gen_cmake_from_manifest.py",
        root / "flows/tools/dump_pyctrace.py",
    )
    text = "\n".join(path.read_text(encoding="utf-8") for path in contract_files)

    assert "pyc6_runtime" in text
    assert "PYC6TRC3" in text
    assert "pyc4_runtime" not in text
    assert "PYC4TRC2" not in text
    assert "PYC4TRC3" not in text

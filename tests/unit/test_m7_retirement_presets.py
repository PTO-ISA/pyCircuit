"""Mutation-check structured CMake preset retirement rules."""

from __future__ import annotations

import copy
import importlib.util
import json
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]
PRESETS = ROOT / "CMakePresets.json"
RETIREMENT_GATE = ROOT / "flows/tools/check_m5_retirement.py"
RETIRED_TARGETS = ("pycc", "pyc-opt", "acc", "acc.py", "acir-opt")
RETIRED_OPTIONS = ("PYC_BUILD_MLIR_TOOLS", "PYC_BUILD_AGENTIC_CIRCUIT_TESTS")


def _gate():
    spec = importlib.util.spec_from_file_location("m7_retirement_gate", RETIREMENT_GATE)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load retirement gate: {RETIREMENT_GATE}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _write(path: Path, value: dict) -> None:
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


def test_current_presets_pass_the_production_retirement_scan() -> None:
    failures = _gate().scan_cmake_presets(PRESETS)

    assert failures == []


def test_production_scan_routes_cmake_presets_through_the_retirement_guard(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    gate = _gate()
    document = copy.deepcopy(json.loads(PRESETS.read_text(encoding="utf-8")))
    document["configurePresets"][0]["cacheVariables"]["PYC_BUILD_MLIR_TOOLS"] = "ON"
    (tmp_path / "CMakePresets.json").write_text(json.dumps(document), encoding="utf-8")
    monkeypatch.setattr(gate, "ROOT", tmp_path)
    monkeypatch.setattr(gate, "PRODUCTION_ROOTS", ())

    failures = gate.scan_production()

    assert any(
        "PYC_BUILD_MLIR_TOOLS" in failure and "preset" in failure.lower()
        for failure in failures
    )


@pytest.mark.parametrize("target", RETIRED_TARGETS)
def test_retirement_scan_rejects_retired_build_preset_targets(
    tmp_path: Path, target: str
) -> None:
    document = copy.deepcopy(json.loads(PRESETS.read_text(encoding="utf-8")))
    document["buildPresets"][0]["targets"].append(target)
    path = tmp_path / "CMakePresets.json"
    _write(path, document)

    failures = _gate().scan_cmake_presets(path)

    assert failures
    assert any(target in failure for failure in failures)


@pytest.mark.parametrize("option", RETIRED_OPTIONS)
def test_retirement_scan_rejects_retired_configure_preset_options(
    tmp_path: Path, option: str
) -> None:
    document = copy.deepcopy(json.loads(PRESETS.read_text(encoding="utf-8")))
    document["configurePresets"][0]["cacheVariables"][option] = "ON"
    path = tmp_path / "CMakePresets.json"
    _write(path, document)

    failures = _gate().scan_cmake_presets(path)

    assert failures
    assert any(option in failure for failure in failures)

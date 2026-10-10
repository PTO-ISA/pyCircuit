"""Mutation-check structured CMake preset retirement rules."""

from __future__ import annotations

import copy
import importlib.util
import json
from pathlib import Path
from types import ModuleType

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]
PRESETS = ROOT / "CMakePresets.json"
RETIREMENT_GATE = ROOT / "tools/check_frontend_retirement.py"
RETIRED_TARGETS = ("pycc", "pyc-opt", "acc", "acc.py", "acir-opt")
RETIRED_OPTIONS = ("PYC_BUILD_MLIR_TOOLS", "PYC_BUILD_AGENTIC_CIRCUIT_TESTS")


def _gate():
    spec = importlib.util.spec_from_file_location(
        "release_preview_retirement_gate", RETIREMENT_GATE
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load retirement gate: {RETIREMENT_GATE}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _write(path: Path, value: dict) -> None:
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


@pytest.fixture
def isolated_production_scan(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> tuple[ModuleType, Path, Path]:
    gate = _gate()
    tools = tmp_path / "tools"
    tools.mkdir()
    scanner = tools / "check_frontend_retirement.py"
    scanner.write_text(RETIREMENT_GATE.read_text(encoding="utf-8"), encoding="utf-8")
    monkeypatch.setattr(gate, "ROOT", tmp_path)
    monkeypatch.setattr(gate, "PRODUCTION_ROOTS", (tools,))
    monkeypatch.setattr(gate, "__file__", str(scanner))
    monkeypatch.setattr(gate, "scan_cmake_presets", lambda _path: [])
    return gate, scanner, tools


@pytest.fixture
def isolated_public_surface_scan(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> tuple[ModuleType, Path]:
    gate = _gate()
    package = tmp_path / "python/pycircuit"
    package.mkdir(parents=True)
    package_init = package / "__init__.py"
    package_init.write_text(
        (ROOT / "python/pycircuit/__init__.py").read_text(encoding="utf-8"),
        encoding="utf-8",
    )
    monkeypatch.setattr(gate, "ROOT", tmp_path)
    return gate, package_init


def test_current_presets_pass_the_production_retirement_scan() -> None:
    failures = _gate().scan_cmake_presets(PRESETS)

    assert failures == []


def test_current_production_tree_passes_the_retirement_scan() -> None:
    assert _gate().scan_production() == []


def test_current_static_public_surface_passes_the_retirement_scan() -> None:
    # The checker owns the literal __all__ declaration. The later u1..u64 loop
    # remains dynamic language-surface authority outside this mechanical test.
    assert _gate().scan_public_surface() == []


def test_public_surface_scan_rejects_a_removed_static_export(
    isolated_public_surface_scan: tuple[ModuleType, Path],
) -> None:
    gate, package_init = isolated_public_surface_scan
    source = package_init.read_text(encoding="utf-8")
    package_init.write_text(source.replace('    "struct",\n', "", 1), encoding="utf-8")

    failures = gate.scan_public_surface()

    assert len(failures) == 1
    assert "public Python exports differ" in failures[0]
    assert "'struct'" not in failures[0]


def test_public_surface_scan_rejects_an_added_retired_static_export(
    isolated_public_surface_scan: tuple[ModuleType, Path],
) -> None:
    gate, package_init = isolated_public_surface_scan
    source = package_init.read_text(encoding="utf-8")
    package_init.write_text(
        source.replace(
            '    "module",\n',
            '    "module",\n    "agentic_circuit",\n',
            1,
        ),
        encoding="utf-8",
    )

    failures = gate.scan_public_surface()

    assert len(failures) == 1
    assert "public Python exports differ" in failures[0]
    assert "'agentic_circuit'" in failures[0]


def test_scanner_rejection_patterns_do_not_flag_the_scanner_itself(
    isolated_production_scan: tuple[ModuleType, Path, Path],
) -> None:
    gate, scanner, _tools = isolated_production_scan
    scanner_lines = scanner.read_text(encoding="utf-8").splitlines()

    assert any(
        pattern.search(line)
        for pattern, _description in gate.RETIRED_TEXT
        for line in scanner_lines
    )
    assert scanner.resolve() not in gate.production_files()
    assert gate.scan_production() == []


def test_retirement_scan_still_checks_other_tools_helpers(
    isolated_production_scan: tuple[ModuleType, Path, Path],
) -> None:
    gate, scanner, tools = isolated_production_scan
    helper = tools / "maintenance_helper.py"
    helper.write_text("QueueGraphGenerator\n", encoding="utf-8")

    production_files = gate.production_files()
    failures = gate.scan_production()

    assert scanner.resolve() not in production_files
    assert helper.resolve() in production_files
    assert failures == ["tools/maintenance_helper.py:1: retired QueueGraph engine"]


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

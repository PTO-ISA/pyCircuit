from __future__ import annotations

import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
pytestmark = pytest.mark.unit

TOP_LEVEL_ROOTS = {
    ".github",
    "benchmarks",
    "cmake",
    "compiler",
    "docs",
    "examples",
    "flows",
    "library",
    "packaging",
    "python",
    "schemas",
    "simulator",
    "tests",
    "third_party",
    "toolchains",
    "tools",
}

FLOW_TOOLS = {
    "build_cpp_manifest.py",
    "check_api_hygiene.py",
    "check_decision_status.py",
    "discover_examples.py",
    "gen_cmake_from_manifest.py",
    "summarize_gate_run.py",
}

PYCIRCUIT_TOOLS = {
    "check-pyc-inventory.py",
    "dump_pyctrace.py",
    "generate-semantic-primitive-registry.py",
    "pyc_module_graph.py",
    "schematic_view.py",
    "visualize_cpp.py",
}


def _tracked_paths() -> list[str]:
    return subprocess.run(
        ("git", "ls-files"),
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=True,
    ).stdout.splitlines()


def test_every_tracked_top_level_root_is_documented() -> None:
    roots = {path.split("/", 1)[0] for path in _tracked_paths() if "/" in path}
    assert roots == TOP_LEVEL_ROOTS

    layout = (ROOT / "docs/development/repository-layout.md").read_text(
        encoding="utf-8"
    )
    for root in sorted(TOP_LEVEL_ROOTS):
        assert f"`{root}/`" in layout


def test_flow_and_product_tools_have_distinct_roots() -> None:
    flow_tools = {
        path.name for path in (ROOT / "flows/tools").glob("*.py") if path.is_file()
    }
    pycircuit_tools = {
        path.name for path in (ROOT / "tools/pycircuit").glob("*.py") if path.is_file()
    }

    assert flow_tools == FLOW_TOOLS
    assert pycircuit_tools == PYCIRCUIT_TOOLS
    assert not list((ROOT / "tools").glob("*.py"))


def test_completed_migration_pages_are_not_active_documents() -> None:
    assert not (ROOT / "docs/acir/migration.md").exists()
    assert not (ROOT / "docs/acir/agentic-circuit-collaboration.md").exists()

    history = (ROOT / "docs/acir/spec/refs/history.md").read_text(encoding="utf-8")
    assert "PTO-ISA/agentic-circuit" in history
    assert "archived" in history


def test_current_generated_dispatch_abi_is_not_named_legacy() -> None:
    current_sources = (
        ROOT / "compiler/acir/lib/CodeGen/EmitCxx.cpp",
        ROOT / "simulator/gfsim/include/gfsim/dispatch.h",
        ROOT / "simulator/gfsim/include/gfsim/object.h",
        ROOT / "simulator/gfsim/system.cpp",
    )
    forbidden = (
        "LegacyDispatch",
        "LegacyActivation",
        "setLegacyDispatch",
        "runLegacy",
    )

    for path in current_sources:
        source = path.read_text(encoding="utf-8")
        for token in forbidden:
            assert token not in source, f"{path.relative_to(ROOT)}: {token}"


def test_acsim_device_kind_never_depends_on_queue_symbol_names() -> None:
    lowering = (
        ROOT / "compiler/acir/lib/Conversion/ACIRToACSim/ACIRToACSim.cpp"
    ).read_text(encoding="utf-8")

    for implicit_name in ('name == "pc"', 'name == "busy"', 'name == "rf"'):
        assert implicit_name not in lowering
    assert 'kind.getValue() == "register"' in lowering
    assert 'kind.getValue() == "regfile"' in lowering

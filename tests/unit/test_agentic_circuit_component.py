from __future__ import annotations

import importlib
import sys
from pathlib import Path

import pytest
import tomllib

REPOSITORY = Path(__file__).resolve().parents[2]
AC_ROOT = REPOSITORY / "python" / "agentic-circuit"
pytestmark = pytest.mark.unit


def test_two_distributions_use_distinct_namespaces_and_bsd_license() -> None:
    pyc_metadata = tomllib.loads(
        (REPOSITORY / "pyproject.toml").read_text(encoding="utf-8")
    )["project"]
    ac_metadata = tomllib.loads(
        (AC_ROOT / "pyproject.toml").read_text(encoding="utf-8")
    )["project"]

    assert pyc_metadata["name"] == "pycircuit-hisi"
    assert ac_metadata["name"] == "agentic-circuit"
    assert "License :: OSI Approved :: BSD License" in pyc_metadata["classifiers"]
    assert ac_metadata["license"] == "BSD-3-Clause"

    sys.path.insert(0, str(AC_ROOT / "src"))
    sys.path.insert(0, str(REPOSITORY / "python" / "pycircuit" / "src"))
    try:
        pycircuit = importlib.import_module("pycircuit")
        agentic_circuit = importlib.import_module("agentic_circuit")
    finally:
        del sys.path[:2]

    assert pycircuit.__name__ == "pycircuit"
    assert agentic_circuit.__name__ == "agentic_circuit"
    assert not hasattr(pycircuit, "agentic_circuit")
    assert pycircuit.module is not agentic_circuit.module


def test_acpy_contract_epoch_is_0_5_across_active_surfaces() -> None:
    metadata = tomllib.loads((AC_ROOT / "pyproject.toml").read_text(encoding="utf-8"))
    assert metadata["tool"]["agentic-circuit"]["contract-epoch"] == "0.5"

    source = (AC_ROOT / "src" / "agentic_circuit" / "_acpy.py").read_text(
        encoding="utf-8"
    )
    assert 'schema: str = "agentic-circuit-acpy"' in source
    assert 'version: str = "0.1"' in source
    assert 'contract_epoch: str = "0.5"' in source

    readme = (REPOSITORY / "README.md").read_text(encoding="utf-8")
    assert "agentic_circuit frontend -> ACPy 0.5 -> ACIR" in readme

    gate_script = (
        REPOSITORY / "flows" / "scripts" / "run_agentic_circuit.sh"
    ).read_text(encoding="utf-8")
    assert '"contract_epoch": "0.5"' in gate_script


def test_consumer_designs_and_adapters_are_out_of_tree() -> None:
    forbidden_roots = (
        "integrations",
        "platforms",
        "examples/pycircuit/linxcore_frontend_pipeline",
    )
    for relative in forbidden_roots:
        assert not (REPOSITORY / relative).exists(), relative

    for relative in (
        "library/cpp/pyc_linxtrace.hpp",
        "library/cpp/pyc_konata.hpp",
    ):
        assert not (REPOSITORY / relative).exists(), relative

    jit_source = (REPOSITORY / "python/pycircuit/src/pycircuit/jit.py").read_text(
        encoding="utf-8"
    )
    assert "_INTERNAL_INLINE_COMPLEXITY_ALLOWLIST" not in jit_source
    assert "integrations/" not in jit_source

    nightly = (REPOSITORY / ".github/workflows/gates-nightly.yml").read_text(
        encoding="utf-8"
    )
    assert "linx_cpu" not in nightly
    assert "integrations/" not in nightly

    repository_config = "\n".join(
        (REPOSITORY / relative).read_text(encoding="utf-8")
        for relative in (".pre-commit-config.yaml", ".gitignore")
    )
    for token in ("integrations/", "XiangShan", "Konata", "outerCube"):
        assert token not in repository_config

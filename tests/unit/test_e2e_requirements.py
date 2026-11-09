"""Keep the end-to-end requirement map executable and free of stale paths."""

from __future__ import annotations

import json
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "docs/testing/e2e-requirements.json"


def test_e2e_requirement_manifest_is_closed() -> None:
    payload = json.loads(MANIFEST.read_text(encoding="utf-8"))
    assert payload["schema"] == "pycircuit-e2e-requirements-v1"
    assert set(payload["entrypoints"]) == {
        "gate", "nightly", "examples_gate", "examples_nightly"
    }
    requirements = payload["requirements"]
    assert requirements
    ids = [row["id"] for row in requirements]
    assert len(ids) == len(set(ids))
    for row in requirements:
        assert row["tier"] in {"gate", "nightly"}
        path = ROOT / row["test"]
        assert path.is_file(), f"stale requirement test path: {row['test']}"


def test_critical_requirements_are_registered_in_official_runners() -> None:
    runner = (ROOT / "tools/run_api_tests.sh").read_text(encoding="utf-8")
    assert "tests/system/test_physical_rtl_export.py" in runner
    assert "tests/system/test_public_emit.py" in runner
    cmake = (ROOT / "tests/compiler/IR/CMakeLists.txt").read_text(encoding="utf-8")
    assert "four-state-constant" in cmake
    assert "value-require-invalid" in cmake

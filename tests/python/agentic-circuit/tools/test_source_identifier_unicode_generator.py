"""Reproducibility tests for the optional source-identifier table generator."""

from __future__ import annotations

import importlib.util
import os
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[4]
GENERATOR = ROOT / "tools/pycircuit/generate_source_identifier_unicode.py"
TABLE = ROOT / "compiler/acir/lib/Compiler/SourceIdentifierUnicodeData.h"
NOTICE = ROOT / "compiler/acir/lib/Compiler/SourceIdentifierUnicodeData.LICENSE.txt"


def _recipe_python() -> str:
    configured = os.environ.get("PYC_UNICODE_GENERATOR_PYTHON")
    if not configured:
        pytest.skip("dedicated CPython 3.14.6/UCD 16 tooling lane is not configured")
    return configured


def _load_generator():
    spec = importlib.util.spec_from_file_location(
        "source_identifier_unicode_generator", GENERATOR
    )
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_regeneration_is_byte_identical_to_the_committed_table(tmp_path: Path) -> None:
    generated = tmp_path / "SourceIdentifierUnicodeData.h"
    completed = subprocess.run(
        [_recipe_python(), str(GENERATOR), "--output", str(generated)],
        text=True,
        capture_output=True,
        check=False,
    )

    assert completed.returncode == 0, completed.stderr
    assert generated.read_bytes() == TABLE.read_bytes()


def test_generator_rejects_an_unapproved_cpython_recipe(monkeypatch) -> None:
    generator = _load_generator()
    exact_recipe = sys.implementation.name == "cpython" and sys.version_info[:3] == (
        3,
        14,
        6,
    )
    if exact_recipe:
        monkeypatch.setattr(generator, "EXPECTED_CPYTHON_VERSION", (0, 0, 0))
        expected = "requires CPython 0.0.0"
    else:
        expected = "requires CPython 3.14.6"

    with pytest.raises(RuntimeError, match=expected):
        generator.generate()


def test_adjacent_notice_records_unicode_and_cpython_provenance() -> None:
    notice = NOTICE.read_text(encoding="utf-8")
    assert "Unicode" in notice
    assert "Python Software Foundation" in notice

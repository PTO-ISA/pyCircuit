"""Focused source-unit generation test for the functional floating example."""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
EXAMPLE = ROOT / "examples" / "functional_float"


def _helper(name: str) -> str | None:
    value = os.environ.get(name)
    return value if value and Path(value).is_file() else None


def _run(*args: str, env: dict[str, str]) -> None:
    result = subprocess.run(
        [sys.executable, "-m", "pycircuit.cli", *args],
        cwd=ROOT,
        env=env,
        text=True,
        capture_output=True,
        check=False,
    )
    assert result.returncode == 0, result.stdout + result.stderr


@pytest.mark.system
def test_functional_float_source_units_compile_link_and_emit(tmp_path: Path) -> None:
    source_compiler = _helper("PYCIRCUIT_SOURCE_COMPILER")
    linker = _helper("PYCIRCUIT_LINKER")
    emitter = _helper("PYCIRCUIT_EMITTER")
    if not (source_compiler and linker and emitter):
        pytest.skip("native pyCircuit source-unit helpers are not configured")

    env = dict(os.environ)
    env.update(
        PYTHONPATH=str(ROOT / "python"),
        PYCIRCUIT_SOURCE_COMPILER=source_compiler,
        PYCIRCUIT_LINKER=linker,
        PYCIRCUIT_EMITTER=emitter,
    )
    units = tmp_path / "units"
    units.mkdir()
    compile_args = {
        "types": ("fp32_types.py", ()),
        "product": ("fp32_product.py", ("types",)),
        "sum": ("fp32_sum.py", ("types",)),
        "product_sum": (
            "fp32_product_sum.py",
            ("types", "product", "sum"),
        ),
    }
    for unit, (source, interfaces) in compile_args.items():
        args = [
            "compile",
            "-c",
            str(EXAMPLE / source),
            "--source-root",
            str(EXAMPLE),
            "--package-prefix",
            "functional_float",
            "-o",
            str(units / unit),
            "--replace",
        ]
        for interface in interfaces:
            args.extend(("-I", str(units / interface)))
        _run(*args, env=env)

    linked = tmp_path / "linked"
    linked.mkdir()
    _run(
        "link",
        str(units / "types"),
        str(units / "product"),
        str(units / "sum"),
        "--top",
        "functional_float.fp32_product.FP32Product",
        "-o",
        str(linked / "product.ac"),
        "--replace",
        env=env,
    )
    generated = tmp_path / "cpp_product"
    _run(
        "emit",
        "--target",
        "cpp",
        "-o",
        str(generated),
        str(linked / "product.ac"),
        "--replace",
        env=env,
    )
    assert (generated / "pycircuit_support.hpp").is_file()
    assert any(generated.rglob("fp32_product.cpp"))

"""Configured LLVM 22 parses representative private Python capture transports."""

from __future__ import annotations

import os
import shutil
import subprocess
from dataclasses import replace
from pathlib import Path

import pytest
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport

pytestmark = pytest.mark.system


def _configured_mlir_opt() -> Path:
    candidates: list[str | None] = [os.environ.get("MLIR_OPT")]
    toolchain = os.environ.get("PYC_TOOLCHAIN_ROOT")
    if toolchain:
        candidates.append(str(Path(toolchain) / "bin/mlir-opt"))
    candidates.append(shutil.which("mlir-opt"))
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return Path(candidate).resolve()
    raise AssertionError(
        "LLVM 22 mlir-opt is required; set MLIR_OPT or PYC_TOOLCHAIN_ROOT"
    )


def _capture_transport(
    tmp_path: Path, source: str, name: str, synthetic_name: str | None
) -> str:
    path = tmp_path / name
    path.write_text(source, encoding="utf-8")
    captured = _capture_source_file(path, source_root=tmp_path)
    if synthetic_name:
        captured = replace(captured, path=tmp_path / synthetic_name)
    return _emit_source_transport(captured)


@pytest.mark.parametrize(
    "name, source, synthetic_name",
    [
        (
            "imports.py",
            "from pycircuit import module, rule\n\n"
            "@module\ndef Leaf():\n"
            "    @rule\n    def idle():\n        return\n\n"
            "    idle()\n",
            None,
        ),
        (
            "literals.py",
            "TRUTH = True\n"
            "INTEGER = 123\n"
            "FLOATING = 1.5\n"
            "COMPLEX = 2j\n"
            "BINARY = b'\\x00\\xff'\n"
            "ELLIPSIS_VALUE = ...\n",
            None,
        ),
        (
            "escaped.py",
            'VALUE = "quote\\" slash\\\\ nul\\x00 ctrl\\x01 piπ sep\u2028"\n',
            'quo"te-π.py',
        ),
        ("huge.py", f"VALUE = 0x{'F' * 5000}\n", None),
    ],
)
def test_configured_llvm22_parses_private_source_transport(
    tmp_path: Path, name: str, source: str, synthetic_name: str | None
) -> None:
    mlir_opt = _configured_mlir_opt()
    version = subprocess.run(
        [str(mlir_opt), "--version"],
        text=True,
        capture_output=True,
        check=False,
    )
    assert version.returncode == 0, version.stderr
    assert "LLVM" in version.stdout
    assert "22.1.8" in version.stdout
    transport = _capture_transport(tmp_path, source, name, synthetic_name)

    completed = subprocess.run(
        [str(mlir_opt), "--allow-unregistered-dialect"],
        input=transport,
        text=True,
        capture_output=True,
        check=False,
    )

    assert completed.returncode == 0, completed.stderr

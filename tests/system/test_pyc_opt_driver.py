from __future__ import annotations

import re
import shutil
import subprocess
from pathlib import Path

import pytest

pytestmark = pytest.mark.system

ROOT = Path(__file__).resolve().parents[2]
TRANSFORMS = ROOT / "compiler" / "mlir" / "lib" / "Transforms"


def _pyc_opt() -> Path:
    candidate = ROOT / ".pycircuit_out" / "toolchain" / "build" / "bin" / "pyc-opt"
    if candidate.is_file():
        return candidate
    installed = shutil.which("pyc-opt")
    if installed is None:
        pytest.skip("pyc-opt is not built")
    return Path(installed)


def _registered_source_arguments() -> set[str]:
    arguments: set[str] = set()
    pattern = re.compile(r'getArgument\(\).*?return\s+"([^"]+)"', re.DOTALL)
    for source in TRANSFORMS.glob("*Pass.cpp"):
        text = source.read_text(encoding="utf-8")
        if "PassRegistration<" not in text:
            continue
        match = pattern.search(text)
        assert match is not None, f"registered pass has no literal argument: {source}"
        arguments.add(match.group(1))
    return arguments


def test_pyc_opt_help_exposes_every_registered_pyc_pass() -> None:
    completed = subprocess.run(
        (_pyc_opt(), "--help"),
        text=True,
        capture_output=True,
        check=False,
    )

    assert completed.returncode == 0, completed.stderr
    missing = sorted(
        argument
        for argument in _registered_source_arguments()
        if f"--{argument}" not in completed.stdout
    )
    assert missing == []

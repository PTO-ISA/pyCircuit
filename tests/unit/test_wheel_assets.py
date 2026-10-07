"""Wheel assets retain current build utilities and reject duplicate source trees."""

from __future__ import annotations

import importlib.util
import os
import shutil
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
pytestmark = pytest.mark.unit


@pytest.fixture
def packager(monkeypatch: pytest.MonkeyPatch):
    directory = ROOT / "packaging/wheel"
    monkeypatch.syspath_prepend(str(directory))
    spec = importlib.util.spec_from_file_location(
        "wheel_asset_packager", directory / "create_wheel.py"
    )
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _package(tmp_path: Path) -> tuple[Path, dict[Path, bytes]]:
    package = tmp_path / "pycircuit"
    package.mkdir()
    (package / "cli.py").write_text("# sole public package\n", encoding="utf-8")
    bundle = package / "_toolchain"
    duplicate = bundle / "share/pycircuit/python/pycircuit"
    duplicate.mkdir(parents=True)
    (duplicate / "cli.py").write_text("# duplicate installed copy\n", encoding="utf-8")
    (bundle / "bin").mkdir()
    (bundle / "bin/pycircuit").write_text("old launcher\n", encoding="utf-8")
    helpers = {}
    # Independent allowlist: real installed build utilities, not Python package
    # sources and not the implementation's BUILD_HELPER_SCRIPTS constant.
    for name in ("prepare_output.py", "verify_example.py"):
        path = bundle / "share/pycircuit/cmake" / name
        path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / "cmake" / name, path)
        helpers[path] = path.read_bytes()
    return package, helpers


def test_wheel_keeps_real_build_helpers_and_one_public_python_copy(
    tmp_path: Path, packager
) -> None:
    package, helpers = _package(tmp_path)
    public = (package / "cli.py").read_bytes()
    packager._drop_bundled_python_copy(package)
    assert (package / "cli.py").read_bytes() == public
    assert not (package / "_toolchain/share/pycircuit/python").exists()
    assert {path: path.read_bytes() for path in helpers} == helpers
    assert set((package / "_toolchain").rglob("*.py")) == set(helpers)
    launcher = (package / "_toolchain/bin/pycircuit").read_text(encoding="utf-8")
    assert "from pycircuit.cli import main" in launcher


@pytest.mark.parametrize(
    "relative",
    [
        "share/pycircuit/cmake/extra.py",
        "share/pycircuit/cmake/nested/prepare_output.py",
        "other/verify_example.py",
        "site-packages/pycircuit/cli.py",
        "share/pycircuit/duplicate/pycircuit/__init__.py",
    ],
)
def test_wheel_rejects_unexpected_helpers_and_other_duplicate_python(
    tmp_path: Path, packager, relative: str
) -> None:
    package, _helpers = _package(tmp_path)
    unexpected = package / "_toolchain" / relative
    unexpected.parent.mkdir(parents=True, exist_ok=True)
    unexpected.write_text("# unsupported extra source\n", encoding="utf-8")
    with pytest.raises(SystemExit, match="unexpected or duplicate Python sources"):
        packager._drop_bundled_python_copy(package)


@pytest.mark.parametrize("name", ["agentic_circuit", "_pycircuit_semantics"])
def test_wheel_rejects_retired_trees_even_without_python_files(
    tmp_path: Path, packager, name: str
) -> None:
    package, _helpers = _package(tmp_path)
    retired = package / "_toolchain/old" / name
    retired.mkdir(parents=True)
    (retired / "README.txt").write_text("retired\n", encoding="utf-8")
    with pytest.raises(SystemExit, match="retired Python tree"):
        packager._drop_bundled_python_copy(package)


def _runtime_prefix(tmp_path: Path) -> Path:
    prefix = tmp_path / "compiler-prefix"
    (prefix / "bin").mkdir(parents=True)
    suffix = ".exe" if os.name == "nt" else ""
    for name in ("pycircuit-source-unit", "pycircuit-link", "pycircuit-emit", "pycircuit-opt"):
        helper = prefix / "bin" / (name + suffix)
        helper.write_bytes(b"executable fixture\n")
        helper.chmod(0o755)
    (prefix / "lib").mkdir()
    library = "pyc6_runtime.lib" if os.name == "nt" else "libpyc6_runtime.a"
    (prefix / "lib" / library).write_bytes(b"runtime fixture\n")
    shutil.copytree(ROOT / "include/gfsim", prefix / "include/gfsim")
    cmake = prefix / "share/pycircuit/cmake"
    cmake.mkdir(parents=True)
    for name in (
        "pycircuitConfig.cmake", "pycircuitConfigVersion.cmake",
        "pycircuitRuntimeTargets.cmake", "pycircuitRuntimeTargets-release.cmake",
    ):
        (cmake / name).write_text("# package fixture\n", encoding="utf-8")
    return prefix


def test_wheel_requires_active_dff_header_and_never_its_retired_predecessor(
    tmp_path: Path, packager
) -> None:
    prefix = _runtime_prefix(tmp_path)
    assert (prefix / "include/gfsim/dff.h").is_file()
    packager._validate_toolchain_files(prefix)
    (prefix / "include/gfsim/dff.h").unlink()
    (prefix / "include/gfsim/SimDFF.h").write_text("retired replacement\n", encoding="utf-8")
    with pytest.raises(SystemExit, match=r"missing runtime header: .*gfsim[/\\]dff\.h"):
        packager._validate_toolchain_files(prefix)

"""Independent starter-content and installed-package checks."""

from __future__ import annotations

import ast
import importlib.util
import os
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

import pytest
from pycircuit._project import init_project, load_project
from pycircuit._spec import _bindings

ROOT = Path(__file__).resolve().parents[2]
ASSETS = {
    "ndf.yaml",
    "ndf.lock",
    "spec/README.md",
    "spec/design-input.md",
    "spec/model-spec.md",
    "spec/acceptance.md",
    "verification/verify.py",
}


@pytest.mark.parametrize("template", ["compute", "stateful", "backpressure"])
def test_starter_contains_parseable_bound_design_and_independent_verifier(
    tmp_path, template
):
    project = init_project(tmp_path / template, template)
    assert ASSETS <= {
        path.relative_to(project).as_posix()
        for path in project.rglob("*")
        if path.is_file()
    }
    bindings = []
    for path in project.rglob("*.py"):
        ast.parse(path.read_text(), filename=str(path))
        bindings.extend(_bindings(path, project))
    assert {
        (item["path"], item["symbol"], item["role"], tuple(item["targets"]))
        for item in bindings
    } == {
        ("model.py", "Worker", "implements", ("MODEL-SPEC-001",)),
        ("model.py", "Pipeline", "implements", ("MODEL-SPEC-002",)),
        ("verification/verify.py", "verify_project", "verifies", ("MODEL-VER-001",)),
    }
    _, config, golden = load_project(project)
    assert config["consumer_start"] == (6 if template == "backpressure" else 0)
    assert golden["final_state"] == [
        4,
        65280 if template == "compute" else 27912,
        4,
        1,
    ]
    assert not list(project.rglob("*.pyc"))
    assert not list(project.rglob("__pycache__"))


def test_template_bytecode_cache_is_not_copied(tmp_path, monkeypatch):
    from pycircuit import _project_template

    package = tmp_path / "package"
    assets = package / "templates/model"
    shutil.copytree(ROOT / "python/pycircuit/templates/model", assets)
    cache = assets / "verification/__pycache__"
    cache.mkdir(exist_ok=True)
    (cache / "verify.cpython-314.pyc").write_bytes(b"\xff\x00bytecode")
    monkeypatch.setattr(_project_template, "__file__", str(package / "template.py"))
    project = init_project(tmp_path / "project")
    assert (project / "verification/verify.py").is_file()
    assert not list(project.rglob("*.pyc"))
    assert not list(project.rglob("__pycache__"))


@pytest.mark.parametrize("packaging_route", ["source", "release"])
def test_wheel_retains_starter_assets_and_installed_init(tmp_path, packaging_route):
    if any(importlib.util.find_spec(name) is None for name in ("setuptools", "wheel")):
        pytest.skip("setuptools and wheel are required for wheel inventory checks")
    stage = tmp_path / "stage"
    stage.mkdir()
    shutil.copyfile(ROOT / "README.md", stage / "README.md")
    shutil.copyfile(ROOT / "LICENSE", stage / "LICENSE")
    if packaging_route == "source":
        shutil.copyfile(ROOT / "pyproject.toml", stage / "pyproject.toml")
        package = stage / "python/pycircuit"
        command = [sys.executable, "-c", "from setuptools import setup; setup()"]
    else:
        shutil.copyfile(ROOT / "packaging/wheel/setup.py", stage / "setup.py")
        package = stage / "pycircuit"
        command = [sys.executable, "setup.py"]
    shutil.copytree(
        ROOT / "python/pycircuit",
        package,
        ignore=shutil.ignore_patterns("__pycache__", "*.pyc", "*.pyo"),
    )
    cache = package / "templates/model/verification/__pycache__"
    cache.mkdir()
    (cache / "verify.cpython-314.pyc").write_bytes(b"\xff\x00bytecode")
    result = subprocess.run(
        [*command, "bdist_wheel", "--dist-dir", str(tmp_path / "wheels")],
        cwd=stage,
        env=dict(os.environ, PYC_WHEEL_VERSION="6.1.0"),
        capture_output=True,
        text=True,
        timeout=120,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    wheel = next((tmp_path / "wheels").glob("*.whl"))
    installed = tmp_path / "installed"
    with zipfile.ZipFile(wheel) as archive:
        names = set(archive.namelist())
        assert {"pycircuit/templates/model/" + name for name in ASSETS} <= names
        assert not any("__pycache__" in name or name.endswith(".pyc") for name in names)
        archive.extractall(installed)
    project = tmp_path / "new-project"
    result = subprocess.run(
        [
            sys.executable,
            "-m",
            "pycircuit.cli",
            "init",
            str(project),
        ],
        cwd=tmp_path,
        env=dict(os.environ, PYTHONPATH=str(installed)),
        capture_output=True,
        text=True,
        timeout=30,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    assert all((project / name).is_file() for name in ASSETS)

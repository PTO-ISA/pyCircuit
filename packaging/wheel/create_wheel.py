#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from platform_tags import wheel_plat_name

try:
    import tomllib
except ModuleNotFoundError:  # pragma: no cover - Python 3.11+ in CI; 3.10 needs tomli.
    try:
        import tomli as tomllib  # type: ignore[no-redef]
    except (
        ModuleNotFoundError
    ) as exc:  # pragma: no cover - depends on local Python version.
        raise SystemExit(
            "wheel packaging requires Python 3.11+ or `tomli` on Python 3.10"
        ) from exc


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def _project_version(repo_root: Path) -> str:
    data = tomllib.loads((repo_root / "pyproject.toml").read_text(encoding="utf-8"))
    return str(data["project"]["version"])


def _ignore_copy(_src: str, names: list[str]) -> set[str]:
    ignored: set[str] = set()
    for name in names:
        if name == "__pycache__" or name.endswith((".pyc", ".pyo")):
            ignored.add(name)
    return ignored


def _copytree(src: Path, dst: Path) -> None:
    shutil.copytree(src, dst, ignore=_ignore_copy, dirs_exist_ok=True)


def _copy_file(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(
        description="Build a platform wheel from a staged pyCircuit toolchain install tree."
    )
    ap.add_argument(
        "--install-dir", required=True, help="Path to staged toolchain install tree"
    )
    ap.add_argument(
        "--out-dir", required=True, help="Directory that will receive the built wheel"
    )
    ap.add_argument(
        "--wheel-version",
        default=None,
        help="Override wheel version (defaults to repo pyproject version)",
    )
    ap.add_argument(
        "--wheel-plat-name",
        default=None,
        help="Optional explicit bdist_wheel platform tag",
    )
    ap.add_argument(
        "--build-root",
        default=None,
        help="Optional parent directory for temporary wheel staging",
    )
    args = ap.parse_args(argv)

    repo_root = _repo_root()
    install_dir = Path(args.install_dir).resolve()
    out_dir = Path(args.out_dir).resolve()
    if not install_dir.is_dir():
        raise SystemExit(f"--install-dir does not exist: {install_dir}")

    version = args.wheel_version or _project_version(repo_root)
    build_root = (
        Path(args.build_root).resolve()
        if args.build_root
        else (repo_root / ".pycircuit_out" / "wheel")
    )
    build_root.mkdir(parents=True, exist_ok=True)
    out_dir.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="stage.", dir=build_root) as tmp:
        stage = Path(tmp)
        package_dir = stage / "pycircuit"
        _copytree(repo_root / "python" / "pycircuit" / "src" / "pycircuit", package_dir)
        _copytree(install_dir, package_dir / "_toolchain")
        bundled_python = package_dir / "_toolchain" / "share" / "pycircuit" / "python"
        if bundled_python.is_dir():
            shutil.rmtree(bundled_python)
        tools_dir = package_dir / "_tools"
        for tool_name in ("gen_cmake_from_manifest.py", "pyc_module_graph.py"):
            _copy_file(repo_root / "flows" / "tools" / tool_name, tools_dir / tool_name)
        _copy_file(repo_root / "LICENSE", stage / "LICENSE")
        _copy_file(repo_root / "README.md", stage / "README.md")
        _copy_file(repo_root / "packaging" / "wheel" / "setup.py", stage / "setup.py")
        _copy_file(
            repo_root / "packaging" / "wheel" / "pyproject.toml",
            stage / "pyproject.toml",
        )

        plat_name = args.wheel_plat_name or wheel_plat_name()

        env = os.environ.copy()
        env["PYC_WHEEL_VERSION"] = version

        cmd = [sys.executable, "setup.py", "bdist_wheel", "--dist-dir", str(out_dir)]
        cmd.extend(["--plat-name", plat_name])

        subprocess.run(cmd, check=True, cwd=stage, env=env)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

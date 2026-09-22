#!/usr/bin/env python3
from __future__ import annotations

import argparse
import importlib.machinery
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


# Helper scripts staged into the wheel's `_tools` directory, as paths relative
# to the repository root. `tests/unit/test_repository_layout.py` asserts that
# every entry still exists, so a relocation cannot leave the wheel build
# referencing a moved file.
WHEEL_TOOL_SOURCES = (
    Path("flows") / "tools" / "gen_cmake_from_manifest.py",
    Path("tools") / "pycircuit" / "pyc_module_graph.py",
)

# The frontend packages the toolchain install tree carries in its own Python
# environment. They are staged at the wheel root, so one wheel installs
# `pycircuit`, `_pycircuit_semantics`, and `agentic_circuit` together and no
# consumer needs a second distribution. The toolchain's own copy is dropped from
# the wheel: it would be a second copy of the same files inside one artifact.
VENDORED_PACKAGES = ("_pycircuit_semantics", "agentic_circuit")

# The Agentic Circuit native compiler bridge. A wheel without it would install
# a frontend that cannot compile, so the build refuses to produce one.
NATIVE_EXTENSION = "_native"

# The wheel is relocated for the platform it is built on, so its bundled
# libraries are found relative to the installed tree instead of at the builder's
# absolute paths (a Homebrew `libLLVM.dylib`, for example). The relocation is
# shared with the SDK archive builder, which needs the same treatment.
PLATFORM_IDS = ("linux-x86_64", "macos-arm64", "windows-x86_64")


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


def _toolchain_site_packages(install_dir: Path) -> Path:
    """Return the single bundled `lib/python<X>/site-packages` environment."""
    matches = sorted(
        (
            path
            for path in (install_dir / "lib").glob("python*/site-packages")
            if path.is_dir()
        ),
        key=lambda path: path.as_posix(),
    )
    if len(matches) != 1:
        raise SystemExit(
            f"expected exactly one bundled python environment under {install_dir}/lib, "
            f"found {[path.name for path in matches]}"
        )
    return matches[0]


def _stage_vendored_packages(install_dir: Path, stage: Path) -> None:
    """Stage the frontend packages so one wheel imports without a second one."""
    site_packages = _toolchain_site_packages(install_dir)
    for package in VENDORED_PACKAGES:
        source = site_packages / package
        if not source.is_dir():
            raise SystemExit(f"bundled toolchain is missing {package}: {source}")
        _copytree(source, stage / package)
    native = [
        path
        for path in (stage / "agentic_circuit").glob(f"{NATIVE_EXTENSION}.*")
        if path.suffix in importlib.machinery.EXTENSION_SUFFIXES
    ]
    if len(native) != 1:
        raise SystemExit(
            "bundled agentic_circuit must carry exactly one native extension; "
            f"found {[path.name for path in native]}"
        )


def _drop_toolchain_frontend_copies(package_dir: Path) -> None:
    """Keep one copy of each frontend package inside the wheel.

    The staged tree already carries them at the wheel root, so the copies under
    the bundled toolchain environment would only duplicate files (including the
    native bridge) inside a single artifact.
    """
    for site_packages in (package_dir / "_toolchain" / "lib").glob(
        "python*/site-packages"
    ):
        for package in VENDORED_PACKAGES:
            duplicate = site_packages / package
            if duplicate.is_dir():
                shutil.rmtree(duplicate)


def _relocate(stage: Path, platform: str) -> None:
    """Rewrite bundled library references so the installed wheel is relocatable.

    The build links the platform's own LLVM, z3, and zstd. Those references are
    absolute on macOS, so a wheel that shipped them verbatim would only run on a
    machine with the builder's exact toolchain paths.
    """
    sdk_tools = Path(__file__).resolve().parents[1] / "sdk"
    sys.path.insert(0, str(sdk_tools))
    import create_platform_manifest  # noqa: PLC0415 - single shared implementation

    create_platform_manifest.relocate_native_dependencies(stage, platform)


def _platform_for(plat_name: str) -> str:
    if "win" in plat_name:
        return "windows-x86_64"
    if plat_name.startswith("macosx"):
        return "macos-arm64"
    return "linux-x86_64"


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
        "--platform",
        choices=PLATFORM_IDS,
        default=None,
        help="Platform profile to relocate for (defaults to the host platform)",
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
        _stage_vendored_packages(install_dir, stage)
        _drop_toolchain_frontend_copies(package_dir)
        bundled_python = package_dir / "_toolchain" / "share" / "pycircuit" / "python"
        if bundled_python.is_dir():
            shutil.rmtree(bundled_python)
        tools_dir = package_dir / "_tools"
        for tool_source in WHEEL_TOOL_SOURCES:
            _copy_file(repo_root / tool_source, tools_dir / tool_source.name)
        _copy_file(repo_root / "LICENSE", stage / "LICENSE")
        _copy_file(repo_root / "README.md", stage / "README.md")
        _copy_file(repo_root / "packaging" / "wheel" / "setup.py", stage / "setup.py")
        _copy_file(
            repo_root / "packaging" / "wheel" / "pyproject.toml",
            stage / "pyproject.toml",
        )

        plat_name = args.wheel_plat_name or wheel_plat_name()
        _relocate(stage, args.platform or _platform_for(plat_name))

        env = os.environ.copy()
        env["PYC_WHEEL_VERSION"] = version

        cmd = [sys.executable, "setup.py", "bdist_wheel", "--dist-dir", str(out_dir)]
        cmd.extend(["--plat-name", plat_name])

        subprocess.run(cmd, check=True, cwd=stage, env=env)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

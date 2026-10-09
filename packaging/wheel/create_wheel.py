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


PRIVATE_HELPERS = (
    "pycircuit-source-unit",
    "pycircuit-link",
    "pycircuit-emit",
    "pycircuit-opt",
)
RUNTIME_HEADERS = (
    "gfsim/model_api.h",
    "gfsim/model_input.h",
    "gfsim/dff.h",
    "gfsim/fifo.h",
    "gfsim/SimModule.h",
    "gfsim/SimSystem.h",
    "gfsim/ObservationSlot.h",
    "gfsim/SimExecutor.h",
    "gfsim/SystemRunner.h",
)
RUNTIME_CMAKE_FILES = (
    "pycircuitConfig.cmake",
    "pycircuitConfigVersion.cmake",
    "pycircuitRuntimeTargets.cmake",
)
RETIRED_PYTHON_TREES = ("_pycircuit_semantics", "agentic_circuit")
BUILD_HELPER_SCRIPTS = frozenset(
    {
        "share/pycircuit/cmake/prepare_output.py",
        "share/pycircuit/cmake/verify_example.py",
    }
)

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


def _runtime_library(install_dir: Path) -> Path:
    filename = "pyc6_runtime.lib" if os.name == "nt" else "libpyc6_runtime.a"
    return install_dir / "lib" / filename


def _helper_suffix() -> str:
    return ".exe" if os.name == "nt" else ""


def _validate_toolchain_files(install_dir: Path) -> None:
    """Reject incomplete compiler/runtime prefixes before producing a wheel."""
    for retired in ("pycc", "pyc-opt", "acc", "acc.py", "agentic-circuit", "acir-opt"):
        for suffix in ("", ".exe"):
            if (install_dir / "bin" / (retired + suffix)).exists():
                raise SystemExit(f"toolchain contains a retired public tool: {retired}")
    for retired in ("lib/cmake/AgenticCircuit", "include/cpp", "include/gfsim/queue.h"):
        if (install_dir / retired).exists():
            raise SystemExit(f"toolchain contains a retired runtime asset: {retired}")
    for helper in PRIVATE_HELPERS:
        path = install_dir / "bin" / f"{helper}{_helper_suffix()}"
        if not path.is_file() or not os.access(path, os.X_OK):
            raise SystemExit(f"toolchain is missing executable helper: {path}")
    runtime = _runtime_library(install_dir)
    if not runtime.is_file():
        raise SystemExit(f"toolchain is missing the unified runtime: {runtime}")
    for header in RUNTIME_HEADERS:
        path = install_dir / "include" / header
        if not path.is_file():
            raise SystemExit(f"toolchain is missing runtime header: {path}")
    cmake_dir = install_dir / "share" / "pycircuit" / "cmake"
    for config in RUNTIME_CMAKE_FILES:
        path = cmake_dir / config
        if not path.is_file():
            raise SystemExit(
                f"toolchain is missing unified Runtime package file: {path}"
            )
    if not any(cmake_dir.glob("pycircuitRuntimeTargets-*.cmake")):
        raise SystemExit("toolchain is missing unified Runtime configuration targets")


def _stage_installed_python(install_dir: Path, package_dir: Path) -> None:
    """Use the installed public package as the wheel's one Python copy."""
    source = install_dir / "share" / "pycircuit" / "python" / "pycircuit"
    if not source.is_dir() or not (source / "cli.py").is_file():
        raise SystemExit(f"toolchain is missing installed pycircuit Python: {source}")
    _copytree(source, package_dir)


def _drop_bundled_python_copy(package_dir: Path) -> None:
    """Avoid shipping the install prefix's Python source a second time."""
    duplicate = package_dir / "_toolchain" / "share" / "pycircuit" / "python"
    if not duplicate.is_dir():
        raise SystemExit(
            f"bundled toolchain is missing its Python install tree: {duplicate}"
        )
    shutil.rmtree(duplicate)
    # The prefix-local CMake launcher must locate the wheel's sole package,
    # even when CMake is launched outside the wheel's Python environment.
    launcher = package_dir / "_toolchain" / "bin" / "pycircuit"
    launcher.write_text(
        "#!/usr/bin/env python3\n"
        "from pathlib import Path\nimport sys\n"
        "sys.path.insert(0, str(Path(__file__).resolve().parents[3]))\n"
        "from pycircuit.cli import main\n"
        "raise SystemExit(main())\n",
        encoding="utf-8",
    )

    bundle = package_dir / "_toolchain"
    # These installed build utilities are separate from the sole public
    # pycircuit package. All other Python payloads remain forbidden here.
    extra_python = [
        path
        for path in bundle.rglob("*.py")
        if path.relative_to(bundle).as_posix() not in BUILD_HELPER_SCRIPTS
    ]
    if extra_python:
        raise SystemExit(
            "bundled toolchain contains unexpected or duplicate Python sources: "
            f"{[str(path.relative_to(bundle)) for path in extra_python[:8]]}"
        )
    for retired in RETIRED_PYTHON_TREES:
        matches = [path for path in bundle.rglob(retired) if path.is_dir()]
        if matches:
            raise SystemExit(
                f"bundled toolchain contains retired Python tree {retired}: {matches}"
            )


def _relocate(stage: Path, platform: str) -> None:
    """Rewrite bundled library references so the installed wheel is relocatable.

    The build links the platform's own LLVM, z3, and zstd. Those references are
    absolute on macOS, so a wheel that shipped them verbatim would only run on a
    machine with the builder's exact toolchain paths.
    """
    sdk_tools = Path(__file__).resolve().parents[1] / "sdk"
    sys.path.insert(0, str(sdk_tools))
    import create_platform_manifest  # noqa: PLC0415 - single shared implementation

    create_platform_manifest.relocate_native_dependencies(
        stage, platform, stage / "pycircuit" / "_toolchain" / "lib"
    )


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
    _validate_toolchain_files(install_dir)
    python_source = install_dir / "share" / "pycircuit" / "python" / "pycircuit"
    if not python_source.is_dir():
        raise SystemExit(
            f"toolchain is missing installed pycircuit Python: {python_source}"
        )

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
        _stage_installed_python(install_dir, package_dir)
        _copytree(install_dir, package_dir / "_toolchain")
        _drop_bundled_python_copy(package_dir)
        _validate_toolchain_files(package_dir / "_toolchain")
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

"""Compile a workspace of Agentic Circuit sources into one linked AC package.

``acc.py`` compiles a single source file into a single AC unit. Reaching a
structured C++ bundle needs two more steps: publishing the source units and
interface headers that make up the linked package, and running the native
``acc`` tool over that directory. This module drives all of it for a Python
caller and returns the emitted bundle, its file inventory, and the parsed
module manifest:

    from agentic_circuit import bundle

    result = bundle.lower_sources("source/core.py", output="build/pipeline")
    result.manifest["instances"]

The native ``acc`` tool is required for the link and bundle steps. It is
located from the ``acc`` argument, then ``AGENTIC_CIRCUIT_ACC``, then ``PATH``.
"""

from __future__ import annotations

import contextlib
import io
import json
import os
import shutil
import subprocess
import tempfile
from collections.abc import Mapping
from dataclasses import dataclass
from pathlib import Path

from . import _acc_py
from ._exit_codes import ExitCode
from ._source_closure import SourceClosureError, capture_source_closure
from ._workspace import WorkspaceConfig, discover_workspace, load_workspace

MODULE_MANIFEST_PATH = Path("share") / "generated" / "module-manifest.json"
_ACC_ENVIRONMENT = "AGENTIC_CIRCUIT_ACC"


class BundleError(RuntimeError):
    """A package could not be linked or emitted into a bundle."""


@dataclass(frozen=True, slots=True)
class ModuleBundle:
    """The bundle emitted from one linked AC package.

    ``package`` is the retained package directory, or ``None`` when the caller
    did not ask to keep it.
    """

    package: Path | None
    bundle: Path
    files: tuple[str, ...]
    manifest: Mapping[str, object]


def _acc_tool(acc: str | os.PathLike[str] | None) -> Path:
    if acc is not None:
        candidate = Path(acc).expanduser()
        if not candidate.is_file():
            raise BundleError(f"acc tool not found: {candidate}")
        return candidate.resolve()
    override = os.environ.get(_ACC_ENVIRONMENT)
    if override:
        candidate = Path(override).expanduser()
        if not candidate.is_file():
            raise BundleError(f"{_ACC_ENVIRONMENT} does not name a file: {candidate}")
        return candidate.resolve()
    discovered = shutil.which("acc")
    if discovered is None:
        raise BundleError(
            "the native acc tool is unavailable; pass acc=<path> or set "
            f"{_ACC_ENVIRONMENT}"
        )
    return Path(discovered).resolve()


def _source_units(architecture: Path, workspace: WorkspaceConfig) -> tuple[Path, ...]:
    """Every imported workspace source that declares exactly one public module."""

    try:
        closure = capture_source_closure(architecture, workspace.root)
    except SourceClosureError as error:
        raise BundleError(str(error)) from error
    units: list[Path] = []
    for entry in closure.entries:
        path = Path(entry.source_file).resolve()
        if path == architecture:
            continue
        try:
            declared = _acc_py._declared_modules(path)
        except (OSError, UnicodeError, SyntaxError) as error:
            raise BundleError(f"cannot inspect {path}: {error}") from error
        if len(declared) == 1:
            units.append(path)
    return tuple(sorted(units))


def _run_acc_py(arguments: list[str]) -> None:
    stream = io.StringIO()
    with contextlib.redirect_stdout(stream), contextlib.redirect_stderr(stream):
        status = _acc_py.main(arguments)
    if status != ExitCode.SUCCESS:
        raise BundleError(f"acc.py failed ({status}): {stream.getvalue().strip()}")


def _compile_package(
    architecture: Path,
    workspace: WorkspaceConfig,
    package: Path,
    system: str | None,
) -> None:
    interface_tree = package / "_interfaces"
    system_arguments = ["--system", system] if system is not None else []
    for unit in _source_units(architecture, workspace):
        unit_relative = unit.relative_to(workspace.root)
        _run_acc_py(
            [
                "-c",
                str(unit),
                "-o",
                str(package / "sources" / unit_relative.with_suffix(".ac")),
                "--header-output",
                str(package / "interfaces" / unit_relative.with_suffix(".ac")),
                "--quiet",
            ]
        )
    _run_acc_py(
        [
            "-c",
            str(architecture),
            "-o",
            str(package / "core.ac"),
            *system_arguments,
            "--quiet",
        ]
    )
    _run_acc_py(
        [
            "-c",
            str(architecture),
            "--unit",
            "interfaces",
            "-o",
            str(interface_tree),
            *system_arguments,
            "--quiet",
        ]
    )
    compiler_interfaces = interface_tree / "_compiler"
    if compiler_interfaces.is_dir():
        destination = package / "interfaces" / "_compiler"
        destination.mkdir(parents=True, exist_ok=True)
        for path in sorted(compiler_interfaces.glob("*.ac")):
            shutil.copy(path, destination / path.name)
    shutil.rmtree(interface_tree, ignore_errors=True)


def lower_sources(
    architecture: str | os.PathLike[str],
    *,
    output: str | os.PathLike[str],
    project: str | os.PathLike[str] | None = None,
    system: str | None = None,
    acc: str | os.PathLike[str] | None = None,
    package: str | os.PathLike[str] | None = None,
) -> ModuleBundle:
    """Compile one workspace architecture into a linked package and bundle.

    The architecture must be a system source: every imported workspace source
    that declares exactly one public ``@ac.module`` is compiled into its own AC
    unit and interface header, the architecture becomes ``core.ac``, and the
    native ``acc`` tool links the directory and emits the structured bundle into
    ``output``. The package directory is removed again unless ``package`` names
    a destination that does not exist yet.
    """

    architecture_path = Path(architecture).expanduser()
    if not architecture_path.is_file():
        raise BundleError(f"architecture not found: {architecture_path}")
    architecture_path = architecture_path.resolve()
    workspace = (
        load_workspace(Path(project))
        if project is not None
        else discover_workspace(architecture_path)
    )
    try:
        architecture_path.relative_to(workspace.root)
    except ValueError as error:
        raise BundleError(
            f"architecture {architecture_path} is outside workspace {workspace.root}"
        ) from error
    declared = _acc_py._declared_modules(architecture_path)
    if declared:
        raise BundleError(
            "a bundle architecture must be a system source, not a module unit: "
            f"{architecture_path} declares {', '.join(declared)}"
        )
    tool = _acc_tool(acc)
    output_path = Path(output).expanduser().resolve()
    if output_path.exists():
        raise BundleError(f"bundle destination already exists: {output_path}")
    if package is not None and Path(package).expanduser().resolve().exists():
        raise BundleError(f"package destination already exists: {package}")

    temporary = tempfile.TemporaryDirectory(prefix="agentic-package-")
    package_path = (
        Path(package).expanduser().resolve()
        if package is not None
        else Path(temporary.name).resolve() / "package"
    )
    package_path.mkdir(parents=True, exist_ok=False)
    try:
        _compile_package(architecture_path, workspace, package_path, system)
        verified = subprocess.run(
            [str(tool), "-c", str(package_path), "-verify"],
            text=True,
            capture_output=True,
            check=False,
        )
        if verified.returncode != 0:
            raise BundleError(f"package verification failed: {verified.stderr.strip()}")
        emitted = subprocess.run(
            [
                str(tool),
                "-c",
                str(package_path),
                "-emit-cpp-bundle",
                "-o",
                str(output_path),
            ],
            text=True,
            capture_output=True,
            check=False,
        )
        if emitted.returncode != 0:
            raise BundleError(f"bundle emission failed: {emitted.stderr.strip()}")
        manifest_path = output_path / MODULE_MANIFEST_PATH
        if not manifest_path.is_file():
            raise BundleError("bundle published no module manifest")
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        files = tuple(
            sorted(
                path.relative_to(output_path).as_posix()
                for path in output_path.rglob("*")
                if path.is_file()
            )
        )
    except BaseException:
        shutil.rmtree(output_path, ignore_errors=True)
        raise
    finally:
        temporary.cleanup()
        if package is None:
            shutil.rmtree(package_path, ignore_errors=True)
            retained = None
        else:
            retained = package_path
    return ModuleBundle(
        package=retained, bundle=output_path, files=files, manifest=manifest
    )

"""Compile one Agentic Circuit Python source file or core to one AC unit."""

from __future__ import annotations

import argparse
import ast
import os
import shutil
import sys
import tempfile
from collections.abc import Sequence
from pathlib import Path

from ._canonical_json import canonical_json_bytes
from ._capture import binding_registry, capture, has_errors
from ._exit_codes import ExitCode
from ._native_api import NativeRequest, run_native_compiler
from ._workspace import UserInputError, discover_workspace, load_workspace


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="acc.py", allow_abbrev=False)
    parser.add_argument(
        "-c",
        dest="architecture",
        type=Path,
        required=True,
        metavar="ARCHITECTURE.py",
    )
    parser.add_argument("-o", dest="output", type=Path, required=True)
    parser.add_argument(
        "--header-output",
        type=Path,
        metavar="MODULE_HEADER.ac",
        help="publish the source-owned ac.module.import header",
    )
    parser.add_argument("--project", type=Path)
    parser.add_argument("--system")
    parser.add_argument("--unit", choices=("core", "interfaces"), default="core")
    parser.add_argument("--quiet", action="store_true")
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--timeout", type=float, default=30.0, metavar="SECONDS")
    return parser


def _select_artifact(native: object, logical_path: str) -> bytes:
    matches = [
        artifact
        for artifact in getattr(native, "artifacts", ())
        if getattr(artifact, "kind", None) == "verified-acir"
        and getattr(artifact, "path", None) == logical_path
    ]
    if len(matches) != 1 or type(getattr(matches[0], "data", None)) is not bytes:
        raise RuntimeError(
            f"compiler produced no unique {logical_path!r} AC unit"
        )
    return matches[0].data


def _select_interface_artifacts(native: object) -> tuple[tuple[str, bytes], ...]:
    result: list[tuple[str, bytes]] = []
    for artifact in getattr(native, "artifacts", ()):
        path = getattr(artifact, "path", None)
        data = getattr(artifact, "data", None)
        if (
            getattr(artifact, "kind", None) == "verified-acir"
            and type(path) is str
            and path.startswith("interfaces/")
            and type(data) is bytes
        ):
            result.append((path.removeprefix("interfaces/"), data))
    return tuple(sorted(result))


def _declared_modules(path: Path) -> tuple[str, ...]:
    tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
    names: list[str] = []
    for node in tree.body:
        if not isinstance(node, ast.FunctionDef):
            continue
        for decorator in node.decorator_list:
            candidate = decorator.func if isinstance(decorator, ast.Call) else decorator
            if (
                isinstance(candidate, ast.Attribute)
                and isinstance(candidate.value, ast.Name)
                and candidate.value.id == "ac"
                and candidate.attr == "module"
            ):
                names.append(node.name)
                break
    return tuple(names)


def _single_declared_module(path: Path) -> str:
    declared = _declared_modules(path)
    if len(declared) != 1:
        raise ValueError(
            "a source translation unit must declare exactly one public "
            f"@ac.module; found {len(declared)}"
        )
    return declared[0]



def _compile(arguments: argparse.Namespace, workspace: object) -> object:
    frontend = capture(arguments, workspace)
    if has_errors(frontend.diagnostics):
        raise UserInputError(frontend.diagnostics[0])
    if frontend.acir is None:
        raise RuntimeError("frontend produced no ACIR")
    native = run_native_compiler(
        NativeRequest(
            acir=frontend.acir,
            stop_after="acir-verify",
            emits=("verified-acir",),
            options=(("binding_registry", binding_registry(workspace.component_roots)),),
        )
    )
    if has_errors(native.diagnostics):
        raise UserInputError(native.diagnostics[0])
    return native


def _publish(destination: Path, data: bytes) -> None:
    _publish_files(((destination, data),))


def _publish_files(files: tuple[tuple[Path, bytes], ...]) -> None:
    if not files:
        return
    destinations = tuple(destination.absolute() for destination, _ in files)
    if len(set(destinations)) != len(destinations):
        raise OSError("output destinations must be distinct")
    for destination in destinations:
        if destination.is_symlink():
            raise OSError("output must not be a symlink")
        if destination.exists():
            raise OSError("output AC unit must not already exist")
        destination.parent.mkdir(parents=True, exist_ok=True)

    staged: list[tuple[Path, Path]] = []
    committed: list[Path] = []
    try:
        for destination, (_, data) in zip(destinations, files, strict=True):
            descriptor, temporary_name = tempfile.mkstemp(
                prefix=f".{destination.name}.",
                suffix=".tmp",
                dir=destination.parent,
            )
            temporary = Path(temporary_name)
            staged.append((temporary, destination))
            with os.fdopen(descriptor, "wb") as stream:
                stream.write(data)
                stream.flush()
                os.fsync(stream.fileno())
        for temporary, destination in staged:
            os.rename(temporary, destination)
            committed.append(destination)
    except OSError:
        for destination in reversed(committed):
            destination.unlink(missing_ok=True)
        raise
    finally:
        for temporary, _ in staged:
            temporary.unlink(missing_ok=True)


def _publish_tree(
    destination: Path, artifacts: tuple[tuple[str, bytes], ...]
) -> None:
    destination = destination.absolute()
    if destination.is_symlink() or destination.exists():
        raise OSError("output interface directory must not already exist")
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = Path(
        tempfile.mkdtemp(prefix=f".{destination.name}.", dir=destination.parent)
    )
    try:
        for logical_path, data in artifacts:
            path = Path(logical_path)
            if path.is_absolute() or not path.parts or ".." in path.parts:
                raise OSError("compiler produced an unsafe interface path")
            target = temporary / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        os.rename(temporary, destination)
    finally:
        if temporary.exists():
            shutil.rmtree(temporary)


def _report_success(arguments: argparse.Namespace, output: Path) -> None:
    if arguments.json:
        sys.stdout.write(
            (
                canonical_json_bytes(
                    {
                        "schema": "agentic-circuit-acc-result",
                        "version": "0.1",
                        "status": "passed",
                        "output": output.resolve().as_posix(),
                    }
                )
                + b"\n"
            ).decode("utf-8")
        )
    elif not arguments.quiet:
        print(f"compiled AC unit to {output.resolve()}")


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_parser()
    arguments = parser.parse_args(argv)
    if arguments.architecture.suffix != ".py":
        parser.error("-c requires a .py architecture")
    if arguments.unit != "interfaces" and arguments.output.suffix != ".ac":
        parser.error("-o requires a .ac output")
    if arguments.unit == "interfaces" and arguments.output.suffix:
        parser.error("interface output must be a directory path")
    if arguments.header_output is not None and arguments.header_output.suffix != ".ac":
        parser.error("--header-output requires a .ac output")
    if arguments.timeout <= 0:
        parser.error("--timeout must be positive")
    try:
        declared_modules = _declared_modules(arguments.architecture)
    except (OSError, UnicodeError, SyntaxError) as error:
        parser.error(str(error))
    if len(declared_modules) > 1:
        parser.error(
            "a source translation unit must declare exactly one public "
            f"@ac.module; found {len(declared_modules)}"
        )
    source_modules = declared_modules
    if arguments.header_output is not None and not source_modules:
        parser.error("--header-output requires one source-owned @ac.module")
    if source_modules and arguments.unit != "core":
        parser.error("--unit is valid only for system/core compilation")
    arguments.source_modules = source_modules
    arguments.module = None
    # ACC compilation is a closed translation-unit operation.  The selected
    # system may use imported structs, rules, and modules, so capture the
    # source closure instead of treating the architecture file as standalone.
    arguments.source_closure = True

    try:
        workspace = (
            load_workspace(arguments.project)
            if arguments.project is not None
            else discover_workspace(arguments.architecture)
        )
        native = _compile(arguments, workspace)
        if source_modules:
            architecture = arguments.architecture.resolve()
            try:
                relative = architecture.relative_to(workspace.root.resolve())
            except ValueError as error:
                raise RuntimeError("source unit escapes the workspace") from error
            logical_path = (
                "sources/" + relative.with_suffix(".ac").as_posix()
            )
            data = _select_artifact(native, logical_path)
            header_data = (
                _select_artifact(
                    native,
                    "interfaces/"
                    + relative.with_suffix(".ac").as_posix(),
                )
                if arguments.header_output is not None
                else None
            )
        elif arguments.unit == "interfaces":
            data = _select_interface_artifacts(native)
            header_data = None
        else:
            data = _select_artifact(native, "core.ac")
            header_data = None
    except UserInputError as error:
        print(f"{error.diagnostic.code}: {error.diagnostic.message}", file=sys.stderr)
        return ExitCode.USER_INPUT
    except (ImportError, RuntimeError) as error:
        print(f"error: compiler is unavailable: {error}", file=sys.stderr)
        return ExitCode.INTERNAL

    try:
        if arguments.unit == "interfaces":
            assert isinstance(data, tuple)
            _publish_tree(arguments.output, data)
        else:
            assert isinstance(data, bytes)
            if arguments.header_output is not None:
                assert isinstance(header_data, bytes)
                _publish_files(
                    (
                        (arguments.output, data),
                        (arguments.header_output, header_data),
                    )
                )
            else:
                _publish(arguments.output, data)
    except OSError as error:
        print(f"error: unable to publish {arguments.output}: {error}", file=sys.stderr)
        return ExitCode.USER_INPUT
    _report_success(arguments, arguments.output)
    return ExitCode.SUCCESS


if __name__ == "__main__":
    raise SystemExit(main())

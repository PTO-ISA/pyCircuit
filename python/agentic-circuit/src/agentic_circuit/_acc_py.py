"""Compile one Agentic Circuit Python source file or core to one AC unit."""

from __future__ import annotations

import argparse
import ast
import json
import os
import shutil
import sys
import tempfile
from collections.abc import Sequence
from pathlib import Path

from ._canonical_json import canonical_json_bytes, validate_ijson_value
from ._commands.check import _has_errors, binding_registry, capture
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
    entry = parser.add_mutually_exclusive_group()
    entry.add_argument("--system")
    entry.add_argument(
        "--specializations-json",
        type=Path,
        metavar="SPECIALIZATIONS.json",
        help="closed specialization set for one Python source translation unit",
    )
    parser.add_argument("--unit", choices=("core", "interfaces"), default="core")
    parser.add_argument(
        "--static-json",
        type=Path,
        metavar="BINDINGS.json",
        help="closed JSON object of typed static argument bindings",
    )
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
            and not path.startswith("interfaces/modules/")
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


def _source_specializations(
    path: Path,
) -> tuple[tuple[str, tuple[tuple[str, object], ...]], ...]:
    try:
        document = json.loads(path.read_text(encoding="utf-8"))
        validate_ijson_value(document)
    except (OSError, UnicodeError, json.JSONDecodeError, ValueError) as error:
        raise ValueError(f"--specializations-json is invalid: {error}") from error
    if type(document) is not dict or set(document) != {
        "schema",
        "version",
        "specializations",
    }:
        raise ValueError("--specializations-json has unexpected fields")
    if (
        document["schema"] != "agentic-circuit-specializations"
        or document["version"] != "0.1"
        or type(document["specializations"]) is not list
        or not document["specializations"]
    ):
        raise ValueError("--specializations-json has an unsupported schema")
    result: list[tuple[str, tuple[tuple[str, object], ...]]] = []
    identities: set[tuple[str, bytes]] = set()
    for item in document["specializations"]:
        if type(item) is not dict or set(item) != {"module", "static"}:
            raise ValueError("source-unit specialization has unexpected fields")
        module = item["module"]
        static = item["static"]
        if type(module) is not str or not module or type(static) is not dict:
            raise ValueError("source-unit specialization fields are invalid")
        canonical = canonical_json_bytes(static)
        identity = (module, canonical)
        if identity in identities:
            raise ValueError("source-unit specialization is duplicated")
        identities.add(identity)
        result.append((module, tuple(sorted(static.items()))))
    return tuple(
        sorted(
            result,
            key=lambda item: (
                item[0],
                canonical_json_bytes(dict(item[1])),
            ),
        )
    )


def _compile(arguments: argparse.Namespace, workspace: object) -> object:
    frontend = capture(arguments, workspace)
    if _has_errors(frontend.diagnostics):
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
    if _has_errors(native.diagnostics):
        raise UserInputError(native.diagnostics[0])
    return native


def _publish(destination: Path, data: bytes) -> None:
    destination = destination.absolute()
    if destination.is_symlink():
        raise OSError("output must not be a symlink")
    if destination.exists():
        raise OSError("output AC unit must not already exist")
    destination.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(
        prefix=f".{destination.name}.", suffix=".tmp", dir=destination.parent
    )
    temporary = Path(temporary_name)
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.rename(temporary, destination)
    finally:
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
    if arguments.specializations_json is not None and arguments.static_json is not None:
        parser.error("--static-json is valid only for system/core compilation")
    if arguments.header_output is not None and arguments.specializations_json is None:
        parser.error("--header-output requires --specializations-json")
    if arguments.header_output is not None and arguments.header_output.suffix != ".ac":
        parser.error("--header-output requires a .ac output")
    if arguments.specializations_json is not None and arguments.unit != "core":
        parser.error("--unit is valid only for system/core compilation")
    if arguments.timeout <= 0:
        parser.error("--timeout must be positive")
    static_arguments: tuple[tuple[str, object], ...] = ()
    if arguments.static_json is not None:
        try:
            document = json.loads(arguments.static_json.read_text(encoding="utf-8"))
            validate_ijson_value(document)
        except (OSError, UnicodeError, json.JSONDecodeError, ValueError) as error:
            parser.error(f"--static-json is invalid: {error}")
        if type(document) is not dict or any(
            type(name) is not str or not name for name in document
        ):
            parser.error("--static-json requires an object with non-empty string keys")
        static_arguments = tuple(sorted(document.items()))
    source_specializations: tuple[
        tuple[str, tuple[tuple[str, object], ...]], ...
    ] = ()
    if arguments.specializations_json is not None:
        try:
            source_specializations = _source_specializations(
                arguments.specializations_json
            )
            declared = {_single_declared_module(arguments.architecture)}
        except (OSError, UnicodeError, SyntaxError, ValueError) as error:
            parser.error(str(error))
        requested = {module for module, _ in source_specializations}
        unknown = sorted(requested - declared)
        if unknown:
            parser.error(
                f"source-unit module {unknown[0]!r} is not declared by -c"
            )
    arguments.static_arguments = static_arguments
    arguments.source_specializations = source_specializations
    arguments.module = None
    # ACC compilation is a closed translation-unit operation.  The selected
    # system may use imported structs, rules, and modules, so capture the
    # source closure instead of treating the architecture file as standalone.
    arguments.jit_source_closure = True

    try:
        workspace = (
            load_workspace(arguments.project)
            if arguments.project is not None
            else discover_workspace(arguments.architecture)
        )
        native = _compile(arguments, workspace)
        if source_specializations:
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
                    "interfaces/modules/"
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
            _publish(arguments.output, data)
            if arguments.header_output is not None:
                assert isinstance(header_data, bytes)
                _publish(arguments.header_output, header_data)
    except OSError as error:
        print(f"error: unable to publish {arguments.output}: {error}", file=sys.stderr)
        return ExitCode.USER_INPUT
    _report_success(arguments, arguments.output)
    return ExitCode.SUCCESS


if __name__ == "__main__":
    raise SystemExit(main())

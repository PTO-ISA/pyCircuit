"""Compile an Agentic Circuit Python architecture to one verified ACIR file."""

from __future__ import annotations

import argparse
import json
import os
import sys
import tempfile
from collections.abc import Sequence
from pathlib import Path

from ._canonical_json import canonical_json_bytes
from ._canonical_json import validate_ijson_value
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
    parser.add_argument(
        "-o", dest="output", type=Path, required=True, metavar="MODEL.ac"
    )
    parser.add_argument("--project", type=Path)
    parser.add_argument("--system")
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


def _publish(destination: Path, data: bytes) -> None:
    destination = destination.absolute()
    if destination.is_symlink():
        raise OSError("output must not be a symlink")
    if destination.exists() and not destination.is_file():
        raise OSError("output must be a regular file")
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
        os.replace(temporary, destination)
    finally:
        temporary.unlink(missing_ok=True)


def _report_success(arguments: argparse.Namespace, output: Path, data: bytes) -> None:
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
        print(f"compiled verified ACIR to {output.resolve()}")


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_parser()
    arguments = parser.parse_args(argv)
    if arguments.architecture.suffix != ".py":
        parser.error("-c requires a .py architecture")
    if arguments.output.suffix != ".ac":
        parser.error("-o requires a .ac output")
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
    arguments.static_arguments = static_arguments

    try:
        workspace = (
            load_workspace(arguments.project)
            if arguments.project is not None
            else discover_workspace(arguments.architecture)
        )
        frontend = capture(arguments, workspace)
        if _has_errors(frontend.diagnostics) or frontend.acir is None:
            for diagnostic in frontend.diagnostics:
                print(f"{diagnostic.code}: {diagnostic.message}", file=sys.stderr)
            return ExitCode.USER_INPUT
        native = run_native_compiler(
            NativeRequest(
                acir=frontend.acir,
                stop_after="topology-closure",
                emits=("verified-acir",),
                options=(
                    ("binding_registry", binding_registry(workspace.component_roots)),
                ),
            )
        )
        if _has_errors(native.diagnostics):
            for diagnostic in native.diagnostics:
                print(f"{diagnostic.code}: {diagnostic.message}", file=sys.stderr)
            return ExitCode.USER_INPUT
        frozen = next(
            (
                artifact
                for artifact in native.artifacts
                if artifact.path == "verified.ac.mlir"
            ),
            None,
        )
        if frozen is None:
            print(
                "error: compiler produced no verified.ac.mlir artifact", file=sys.stderr
            )
            return ExitCode.INTERNAL
        data = frozen.data
    except UserInputError as error:
        print(f"{error.diagnostic.code}: {error.diagnostic.message}", file=sys.stderr)
        return ExitCode.USER_INPUT
    except (ImportError, RuntimeError) as error:
        print(f"error: compiler is unavailable: {error}", file=sys.stderr)
        return ExitCode.INTERNAL

    try:
        _publish(arguments.output, data)
    except OSError as error:
        print(f"error: unable to publish {arguments.output}: {error}", file=sys.stderr)
        return ExitCode.USER_INPUT
    _report_success(arguments, arguments.output, data)
    return ExitCode.SUCCESS


if __name__ == "__main__":
    raise SystemExit(main())

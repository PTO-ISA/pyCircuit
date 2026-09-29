"""Public driver commands for the single-route migration.

The driver only orchestrates. It captures one stable source snapshot, hands
explicit inputs to a private repo-local helper, and publishes the verified
result under the shared publication protocol. It parses no MLIR, guesses no
imports, and selects no second semantic chain.
"""

from __future__ import annotations

import functools
import json
import os
import shutil
import subprocess
import tempfile
from collections.abc import Mapping, Sequence
from pathlib import Path

from . import packaged_toolchain
from ._publication import (
    _PublicationError,
    _PublicationFileSystem,
    _PublicationInput,
    _PublicationOutput,
    _PublicationResult,
    _paths_for,
    _publication_lock_set,
    _publication_owner_program,
    _publish_file,
    _require_initialized_control,
)
from ._source_compile import _SourceCompileResult, _compile_source_unit
from ._source_unit_files import (
    _discover_source_unit_owner,
    _read_full_source_unit,
    _validate_full_source_unit,
)

# Each entry names the bundled executable and the environment override a
# development tree uses when the toolchain bundle is not installed.
_HELPERS: dict[str, tuple[str, str]] = {
    "source-unit": ("acir-source-unit-harness", "ACIR_SOURCE_UNIT_HARNESS"),
    "design": ("acir-design-harness", "ACIR_DESIGN_HARNESS"),
}


class _DriverError(RuntimeError):
    """A public driver command could not be carried out."""


def _native_helper(kind: str) -> Path:
    """Resolve one private native helper, or fail with the exact override."""

    name, variable = _HELPERS[kind]
    override = os.environ.get(variable)
    if override:
        path = Path(override)
        if not path.is_file():
            raise _DriverError(f"{variable} does not name a file: {path}")
        return path
    bundled = packaged_toolchain.tool_executable(name)
    if bundled is not None:
        return bundled
    raise _DriverError(
        f"cannot locate `{name}`: the bundled toolchain is not installed in "
        f"this tree and {variable} is not set"
    )


def _require_published_unit(directory: str | Path, *, role: str) -> Path:
    """Name the real cause before the private receipt reader is called.

    That reader reports any read failure, including a missing file, as an
    unstable encoding, which is misleading on the public command surface.
    """

    path = Path(directory)
    if not path.is_dir():
        raise _DriverError(f"{role} is not a directory: {path}")
    if not (path / "unit.json").is_file():
        raise _DriverError(
            f"{role} is not a published source unit (no unit.json): {path}"
        )
    return path


def compile_command(
    *,
    source: str | Path,
    source_root: str | Path,
    output: str | Path,
    package_prefix: str = "",
    interface_units: Sequence[str | Path] = (),
    replace: bool = False,
) -> _SourceCompileResult:
    """``pycircuit compile``: one source into one published source unit."""

    for unit in interface_units:
        _require_published_unit(unit, role="compile interface unit")
    return _compile_source_unit(
        source,
        source_root=source_root,
        package=package_prefix,
        native_compiler=_native_helper("source-unit"),
        output=output,
        interface_units=tuple(interface_units),
        replace=replace,
    )


def _reject_parameter_bindings(parameters: str | Path | None) -> None:
    """Fail closed until static-parameter specialization is implemented.

    An omitted ``--parameters`` and an explicit empty array both mean "no
    bindings", which is the only case the native linker supports today.
    """

    if parameters is None:
        return
    text = Path(parameters).read_text(encoding="utf-8")
    try:
        value = json.loads(text)
    except json.JSONDecodeError as error:
        raise _DriverError(f"parameter bindings are not valid JSON: {error}") from error
    if type(value) is not list:
        raise _DriverError("parameter bindings must be an ordered JSON array")
    if value:
        raise _DriverError(
            "static parameter bindings are not implemented yet; "
            "--parameters must be an empty array or omitted"
        )


def _program_owner_from_report(report: object) -> dict[str, object]:
    """Turn the private entry-owner report into a publication owner."""

    if type(report) is not dict or set(report) != {"source", "definition"}:
        raise _DriverError("link helper entry owner report is not closed")
    source = report["source"]
    if type(source) is not dict or set(source) != {"package", "path"}:
        raise _DriverError("link helper entry owner report has no source owner")
    package = source["package"]
    path = source["path"]
    definition = report["definition"]
    if (
        type(package) is not str
        or type(path) is not str
        or type(definition) is not str
    ):
        raise _DriverError("link helper entry owner report is not textual")
    return _publication_owner_program(
        package=package, path=path, definition=definition
    )


def _validate_published_program(path: Path, owner: Mapping[str, object]) -> None:
    """Intrinsic file-level validation of a published program artifact.

    The publication protocol validates an existing artifact with this same
    validator before ``--replace`` installs over it, so the check cannot depend
    on the new linker output. Python does not adjudicate the linked semantics
    here: the linker produced and verified this artifact under the same lock
    set. That the existing artifact is one this driver published is enforced
    separately, because ``program.ac`` carries no owner record.
    """

    if not path.is_file() or path.is_symlink():
        raise _DriverError("published program is not a regular file")
    if not path.read_text(encoding="utf-8").strip():
        raise _DriverError("published program is empty")


def link_command(
    *,
    units: Sequence[str | Path],
    top: str,
    output: str | Path,
    parameters: str | Path | None = None,
    replace: bool = False,
) -> _PublicationResult:
    """``pycircuit link``: one closed unit set into one published program."""

    _reject_parameter_bindings(parameters)
    fs = _PublicationFileSystem()
    destination = Path(output)
    harness = _native_helper("design")

    # Discovery reads declared owners only to name the expected inputs; the lock
    # set re-validates every owner and artifact under the lock, so a
    # substitution between discovery and locking fails closed.
    inputs: list[_PublicationInput] = []
    directories: dict[tuple[str, str], Path] = {}
    owners: dict[tuple[str, str], dict[str, object]] = {}
    for unit in units:
        directory = _require_published_unit(unit, role="link unit")
        owner = _discover_source_unit_owner(directory)
        declared = owner["source"]
        assert isinstance(declared, dict)
        key = (str(declared["package"]), str(declared["path"]))
        if key in directories:
            raise _DriverError("two link units declare the same source")
        directories[key] = directory
        owners[key] = owner
        inputs.append(
            _PublicationInput(
                destination=directory,
                owner=owner,
                # link reads the full body/header closure, unlike compile, which
                # is satisfied by a managed interface.
                stable_validate=_validate_full_source_unit,
                recovery_validate=_validate_full_source_unit,
            )
        )
    if not inputs:
        raise _DriverError("link requires at least one explicitly listed unit")

    if replace and (destination.exists() or destination.is_symlink()):
        # C3-C forbids --replace touching anything this driver did not publish,
        # but a program carries no owner record, so the only invariant Python can
        # check is that a publication control directory names this destination.
        # That is a typo and accident guard, not proof of a prior publication:
        # the control directory is also bootstrapped by any earlier command that
        # failed here. Requiring it before the lock set runs avoids creating one
        # on this path, and is_symlink() catches a dangling link, whose target
        # exists() cannot see.
        try:
            _require_initialized_control(_paths_for(destination, fs), fs)
        except _PublicationError as error:
            raise _DriverError(
                "refusing to replace a path with no publication control "
                f"directory: {destination}"
            ) from error

    scratch = Path(tempfile.mkdtemp(prefix="pycircuit-link-")).resolve()
    try:
        with _publication_lock_set(
            inputs=inputs,
            outputs=[
                _PublicationOutput(
                    destination=destination,
                    recovery_validate=_validate_published_program,
                )
            ],
            filesystem=fs,
        ) as locks:
            command = [str(harness)]
            for index, key in enumerate(sorted(directories)):
                view = locks.snapshot(
                    directories[key],
                    functools.partial(
                        _read_full_source_unit, owner=owners[key], filesystem=fs
                    ),
                )
                # The linker consumes scratch copies of the in-lock snapshots and
                # never reopens a provider file.
                body = scratch / f"{index}.body.ac"
                header = scratch / f"{index}.interface.ac"
                body.write_text(view.body, encoding="utf-8")
                header.write_text(view.interface, encoding="utf-8")
                command += ["--body", str(body), "--header", str(header)]
            program = scratch / "program.ac"
            report = scratch / "entry-owner.json"
            command += [
                "--top",
                top,
                "--target",
                "final",
                "--output",
                str(program),
                "--entry-owner-out",
                str(report),
            ]
            completed = subprocess.run(
                command, text=True, capture_output=True, check=False
            )
            if completed.returncode != 0:
                detail = completed.stderr.strip() or completed.stdout.strip()
                raise _DriverError(f"native link rejected the unit set: {detail}")
            if not program.is_file() or not report.is_file():
                raise _DriverError("native link produced no program or owner report")
            text = program.read_text(encoding="utf-8")
            owner = _program_owner_from_report(
                json.loads(report.read_text(encoding="utf-8"))
            )

            def build(stage: Path) -> None:
                stage.write_text(text, encoding="utf-8")

            return _publish_file(
                destination,
                owner=owner,
                build=build,
                validate=_validate_published_program,
                replace=replace,
                filesystem=fs,
                locks=locks,
            )
    finally:
        shutil.rmtree(scratch, ignore_errors=True)

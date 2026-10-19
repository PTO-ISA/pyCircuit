"""Public driver commands for pycircuit source compilation and emission.

The driver only orchestrates. It captures one stable source snapshot, hands
explicit inputs to a private repo-local helper, and publishes the verified
result under the shared publication protocol. It parses no MLIR, guesses no
imports, and selects no second semantic chain.
"""

from __future__ import annotations

import functools
import json
import re
import shutil
import subprocess
import tempfile
from collections.abc import Mapping, Sequence
from pathlib import Path

from . import _native_verify
from ._publication import (
    _paths_conflict,
    _paths_for,
    _publication_lock_set,
    _publication_owner_program,
    _PublicationError,
    _PublicationFileSystem,
    _PublicationInput,
    _PublicationOutput,
    _PublicationResult,
    _publish_directory,
    _publish_file,
    _require_initialized_control,
)
from ._publication_fs import _PublicationFileSystemError
from ._source_capture import _read_stable_file_bytes
from ._source_compile import _compile_source_unit, _SourceCompileResult
from ._source_unit_files import (
    _discover_source_unit_owner,
    _read_full_source_unit,
    _reject_duplicate_keys,
    _validate_full_source_unit,
)

# Each entry names the bundled executable and the environment override a
# development tree uses when the toolchain bundle is not installed.
_HELPERS: dict[str, tuple[str, str]] = _native_verify._HELPERS


class _DriverError(RuntimeError):
    """A public driver command could not be carried out."""


def _native_helper(kind: str) -> Path:
    """Resolve one private native helper, or fail with the exact override."""

    return _native_verify.native_helper(kind)


def _require_published_unit(directory: str | Path, *, role: str) -> Path:
    """Accept a unit directory, including one whose transaction is unfinished.

    A unit whose last publication did not finish has no receipt at its
    destination yet, because the old target was moved aside. That is a
    legitimate managed state: discovery reads the validated journal owner and the
    lock set recovers the unit, so the preflight must not report it missing.
    """

    path = Path(directory)
    if path.is_dir() and (path / "unit.json").is_file():
        return path
    if _managed_transaction_exists(path):
        return path
    if not path.is_dir():
        raise _DriverError(f"{role} is not a directory: {path}")
    raise _DriverError(f"{role} is not a published source unit (no unit.json): {path}")


def _managed_transaction_exists(path: Path) -> bool:
    filesystem = _PublicationFileSystem()
    if not path.parent.is_dir():
        return False
    try:
        paths = _paths_for(path, filesystem)
    except (_PublicationError, _PublicationFileSystemError):
        return False
    return filesystem.kind(paths.control) == "directory"


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


def _read_parameter_bindings(parameters: str | Path | None) -> list[dict[str, str]]:
    """Read one stable, closed-schema snapshot of named Integer overrides."""
    if parameters is None:
        return []
    try:
        text = _read_stable_file_bytes(Path(parameters)).decode("utf-8")
        value = json.loads(text, object_pairs_hook=_reject_duplicate_keys)
    except (OSError, UnicodeDecodeError, ValueError) as error:
        raise _DriverError(
            f"parameter bindings are not stable strict JSON: {error}"
        ) from error
    if type(value) is not list:
        raise _DriverError("parameter bindings must be a JSON array")
    names: set[str] = set()
    for entry in value:
        if type(entry) is not dict or set(entry) != {"name", "value"}:
            raise _DriverError("each parameter binding requires exactly name and value")
        name, number = entry["name"], entry["value"]
        if type(name) is not str or not name or name in names:
            raise _DriverError("parameter binding names must be nonempty and unique")
        if type(number) is not str or re.fullmatch(r"0|-?[1-9][0-9]*", number) is None:
            raise _DriverError(
                "parameter values must be canonical signed decimal strings"
            )
        names.add(name)
    return value


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
    if type(package) is not str or type(path) is not str or type(definition) is not str:
        raise _DriverError("link helper entry owner report is not textual")
    return _publication_owner_program(package=package, path=path, definition=definition)


def _validate_published_program(path: Path, owner: Mapping[str, object]) -> None:
    """Publication callback validating one linked program artifact.

    The protocol calls this for the existing artifact before ``--replace``, for
    the new staging file, for the installed target and during recovery, each time
    with the owner that step is about: the request's owner when replacing, the
    journal's own owner when recovering. The artifact is judged by the shared
    native verifier, and its verified root owner must equal that owner exactly.
    """

    if not path.is_file() or path.is_symlink():
        raise _PublicationError("published program is not a regular file")
    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError as error:
        # An undecodable target is corruption, and must report in that family
        # rather than through the protocol's generic validation wrapper.
        raise _PublicationError("published program is not UTF-8 text") from error
    if not text.strip():
        raise _PublicationError("published program is empty")
    actual = _native_verify.verify_program_owner(path)
    if actual != dict(owner):
        raise _PublicationError("published program owner does not match")


def link_command(
    *,
    units: Sequence[str | Path] = (),
    top: str | None = None,
    output: str | Path | None = None,
    parameters: str | Path | None = None,
    reset_semantics: str | None = None,
    target: str | None = None,
    from_anchor: str | Path | None = None,
    verify_only: bool = False,
    replace: bool = False,
    filesystem: _PublicationFileSystem | None = None,
) -> _PublicationResult:
    """``pycircuit link``: one closed unit set into one published program.

    ``filesystem`` exists so a test can drive the real publication fault hooks,
    exactly as the private per-source entry already allows.
    """

    if from_anchor is not None:
        if (
            units
            or top is not None
            or parameters is not None
            or reset_semantics is not None
        ):
            raise _DriverError(
                "--from-anchor forbids units, --top, --parameters and --reset-semantics"
            )
        if verify_only:
            if output is not None or target is not None or replace:
                raise _DriverError(
                    "--verify-only forbids output and transformation options"
                )
        elif target != "final" or output is None:
            raise _DriverError(
                "--from-anchor refinement requires --target final and -o"
            )
        return _link_from_anchor(
            from_anchor,
            output=output,
            verify_only=verify_only,
            replace=replace,
            filesystem=filesystem,
        )
    if verify_only:
        raise _DriverError("--verify-only requires --from-anchor")
    if top is None or output is None:
        raise _DriverError("source link requires --top and -o")
    target = target or "final"
    if target not in ("final", "anchor"):
        raise _DriverError("link target must be final or anchor")
    reset_semantics = reset_semantics or "strict"
    if reset_semantics not in ("strict", "if_one"):
        raise _DriverError("reset semantics must be strict or if_one")
    parameter_bindings = _read_parameter_bindings(parameters)
    fs = filesystem or _PublicationFileSystem()
    destination = Path(output)
    harness = _native_helper("design")
    from ._anchor_files import _capture_anchor, _validate_anchor

    validate_output = (
        _validate_anchor if target == "anchor" else _validate_published_program
    )

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
        # Replacement must not touch anything this driver did not publish,
        # but a program carries no owner record, so the only invariant Python can
        # check is that a publication control directory names this destination.
        # That is a typo and accident guard, not proof of a prior publication:
        # the control directory is also bootstrapped by any earlier command that
        # reached the lock set at this path and then failed. Requiring it before
        # the lock set runs avoids creating one here, and is_symlink() catches a
        # dangling link, whose target exists() cannot see.
        try:
            _require_initialized_control(_paths_for(destination, fs), fs)
        except _PublicationError as error:
            raise _DriverError(
                "refusing to replace a path without a publication control "
                f"directory naming it: {destination}"
            ) from error

    scratch = Path(tempfile.mkdtemp(prefix="pycircuit-link-")).resolve()
    try:
        parameter_snapshot = scratch / "parameters.json"
        if parameter_bindings:
            parameter_snapshot.write_text(
                json.dumps(parameter_bindings), encoding="utf-8"
            )
        with _publication_lock_set(
            inputs=inputs,
            outputs=[
                _PublicationOutput(
                    destination=destination,
                    recovery_validate=validate_output,
                )
            ],
            filesystem=fs,
        ) as locks:
            command = [str(harness), "--reset-semantics", reset_semantics]
            if parameter_bindings:
                command += ["--parameters", str(parameter_snapshot)]
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
            program = scratch / ("anchor" if target == "anchor" else "design_top.ac")
            report = scratch / "entry-owner.json"
            command += [
                "--top",
                top,
                "--target",
                target,
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
            if not report.is_file() or (target == "final" and not program.is_file()):
                raise _DriverError("native link produced no program or owner report")
            owner = _program_owner_from_report(_read_entry_owner_report(report))
            if target == "anchor":
                owner["kind"] = "anchor"
                manifest = _capture_anchor(program)
                if manifest["owner"] != owner:
                    raise _DriverError(
                        "native anchor manifest and owner report disagree"
                    )
                return _publish_directory(
                    destination,
                    owner=owner,
                    build=lambda stage: _capture_anchor(program, stage),
                    validate=_validate_anchor,
                    replace=replace,
                    filesystem=fs,
                    locks=locks,
                )
            if not program.is_file():
                raise _DriverError("native link produced no program")
            text = program.read_text(encoding="utf-8")

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


def _read_entry_owner_report(path: Path) -> object:
    from ._anchor_files import _MANIFEST_LIMIT, _read_bounded, _unique_object

    try:
        return json.loads(
            _read_bounded(path, _MANIFEST_LIMIT, _PublicationFileSystem()).decode(
                "utf-8"
            ),
            object_pairs_hook=_unique_object,
        )
    except (ValueError, UnicodeError, RecursionError) as error:
        raise _DriverError(
            f"native entry owner report is not bounded strict JSON: {error}"
        ) from error


def _link_from_anchor(
    anchor: str | Path,
    *,
    output: str | Path | None,
    verify_only: bool,
    replace: bool,
    filesystem: _PublicationFileSystem | None,
) -> _PublicationResult:
    from ._anchor_files import _capture_anchor, _discover_anchor_owner, _validate_anchor

    fs = filesystem or _PublicationFileSystem()
    source = fs.absolute(anchor)
    owner, managed = _discover_anchor_owner(source)
    paths = _paths_for(source, fs)
    outputs = []
    if output is not None:
        output_paths = _paths_for(output, fs)
        for left in (paths.destination, paths.control):
            for right in (output_paths.destination, output_paths.control):
                if _paths_conflict(left, right):
                    raise _PublicationError(
                        "anchor input and publication output overlap"
                    )
        if replace and fs.kind(output_paths.destination) is not None:
            _require_initialized_control(output_paths, fs)
        outputs.append(_PublicationOutput(output, _validate_published_program))
    inputs = (
        [_PublicationInput(source, owner, _validate_anchor, _validate_anchor)]
        if managed
        else []
    )
    with tempfile.TemporaryDirectory(prefix="pycircuit-anchor-link-") as scratch_name:
        scratch = Path(scratch_name).resolve()
        snapshot = scratch / "anchor"
        with _publication_lock_set(
            inputs=inputs, outputs=outputs, filesystem=fs
        ) as locks:
            if managed:
                manifest = locks.snapshot(
                    source, lambda path: _capture_anchor(path, snapshot)
                )
            else:
                if fs.kind(paths.control) is not None:
                    raise _PublicationError(
                        "anchor controls appeared before portable snapshot"
                    )
                manifest = _capture_anchor(source, snapshot)
                if fs.kind(paths.control) is not None:
                    raise _PublicationError(
                        "anchor controls appeared while capturing portable snapshot"
                    )
            if manifest["owner"] != owner:
                raise _PublicationError("anchor owner changed while capturing")
            actual_owner = _native_verify.verify_anchor_owner(snapshot)
            if actual_owner != owner:
                raise _PublicationError("native anchor owner does not match manifest")
            if verify_only:
                return _PublicationResult(committed=False)
            program, report = scratch / "final.ac", scratch / "entry-owner.json"
            _native_verify._run(
                [
                    str(_native_helper("design")),
                    "--from-anchor",
                    str(snapshot),
                    "--target",
                    "final",
                    "--output",
                    str(program),
                    "--entry-owner-out",
                    str(report),
                ],
                "anchor refinement",
            )
            program_owner = _program_owner_from_report(_read_entry_owner_report(report))
            if program_owner != {**owner, "kind": "program"}:
                raise _PublicationError(
                    "refined program owner differs from saved anchor"
                )
            if not program.is_file() or program.is_symlink():
                raise _DriverError(
                    "anchor refinement produced no ordinary final artifact"
                )
            text = program.read_text(encoding="utf-8")
            return _publish_file(
                output,
                owner=program_owner,
                build=lambda stage: stage.write_text(text, encoding="utf-8"),
                validate=_validate_published_program,
                replace=replace,
                filesystem=fs,
                locks=locks,
            )

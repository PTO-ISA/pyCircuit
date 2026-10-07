"""Private single-source compile orchestration.

One call compiles exactly one Python source with explicitly supplied managed
interface units and atomically publishes the four-file source unit:

    <stem>.ac  <stem>.interface.ac  <stem>.d  unit.json

The stem comes from the source file name. Python captures, schedules, and
publishes; every semantic decision stays with the native MLIR compiler. The
whole command runs under one publication lock set, so interface snapshots, the
native compile, and publication cannot interleave with a provider replacement.

Like every other publication in this package, the caller must ensure the output
directory's parent exists and that no path component is a symlink: the
publication protocol rejects symlinked ancestors (on macOS `/var` is one, so
resolve the output root first). The publication control directory is created
next to the destination.
"""

from __future__ import annotations

import functools
import json
import subprocess
import sys
import tempfile
from collections.abc import Callable, Mapping, Sequence
from pathlib import Path

from ._publication import (
    _publication_lock_set,
    _publication_owner_source_unit,
    _PublicationError,
    _PublicationInput,
    _PublicationOutput,
    _PublicationResult,
    _publish_artifact,
)
from ._publication_fs import _PublicationFileSystem
from ._source_capture import _capture_source_file
from ._source_transport import _emit_source_transport
from ._source_unit_files import (
    _discover_source_unit_owner,
    _read_header_view,
    _validate_full_source_unit,
    _validate_source_unit_header,
)

__all__ = ["_SourceCompileResult", "_compile_source_unit"]


class _SourceCompileResult:
    """The publication result plus the published unit identity."""

    __slots__ = ("destination", "owner", "body", "interface", "depfile", "result")

    def __init__(
        self,
        *,
        destination: Path,
        owner: Mapping[str, object],
        body: str,
        interface: str,
        depfile: str,
        result: _PublicationResult,
    ) -> None:
        self.destination = destination
        self.owner = owner
        self.body = body
        self.interface = interface
        self.depfile = depfile
        self.result = result

    def __repr__(self) -> str:  # pragma: no cover - diagnostics only
        return f"_SourceCompileResult(destination={self.destination!r})"


def _reject_duplicate_keys(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            raise _PublicationError(f"duplicate JSON key in compiler response: {key!r}")
        result[key] = value
    return result


def _read_consumed_dependencies(path: Path) -> list[tuple[str, str]]:
    """Parse the native compiler's consumed-interface response."""

    try:
        text = path.read_text(encoding="utf-8")
    except OSError as error:
        raise _PublicationError(
            "native compiler did not report its consumed interfaces"
        ) from error
    try:
        value = json.loads(text, object_pairs_hook=_reject_duplicate_keys)
    except (json.JSONDecodeError, ValueError) as error:
        raise _PublicationError(
            "native compiler consumed-interface response is not strict JSON"
        ) from error
    if type(value) is not list:
        raise _PublicationError("native compiler response is not a list")
    consumed: list[tuple[str, str]] = []
    for entry in value:
        if type(entry) is not dict or set(entry) != {"package", "path"}:
            raise _PublicationError(
                "native compiler response entry fields are not closed"
            )
        package = entry["package"]
        source_path = entry["path"]
        if type(package) is not str or type(source_path) is not str:
            raise _PublicationError("native compiler response entry is not textual")
        consumed.append((package, source_path))
    if len(set(consumed)) != len(consumed):
        raise _PublicationError("native compiler response repeats a dependency")
    return consumed


def _make_escape(value: str) -> str:
    """Escape one path for a Make/CMake-consumed depfile."""

    escaped = value.replace("\\", "\\\\")
    escaped = escaped.replace(" ", "\\ ")
    escaped = escaped.replace("#", "\\#")
    escaped = escaped.replace("$", "$$")
    return escaped


def _depfile_text(target: Path, prerequisites: Sequence[Path]) -> str:
    body = " ".join(_make_escape(str(item)) for item in prerequisites)
    return f"{_make_escape(str(target))}: {body}\n"


def _receipt_text(*, package: str, source_path: str, stem: str) -> str:
    return (
        json.dumps(
            {
                "kind": "pycircuit-source-unit",
                "source": {"package": package, "path": source_path},
                "files": {
                    "body": f"{stem}.ac",
                    "interface": f"{stem}.interface.ac",
                    "depfile": f"{stem}.d",
                },
            },
            indent=2,
        )
        + "\n"
    )


def _compile_source_unit(
    source: str | Path,
    *,
    source_root: str | Path,
    package: str,
    native_compiler: str | Path,
    output: str | Path,
    interface_units: Sequence[str | Path] = (),
    replace: bool = False,
    cancelled: Callable[[str], bool] | None = None,
    filesystem: _PublicationFileSystem | None = None,
) -> _SourceCompileResult:
    """Compile one source and atomically publish its source-unit directory."""

    fs = filesystem or _PublicationFileSystem()
    destination = Path(output)
    compiler = Path(native_compiler)

    # Discovery reads the declared owners only to name the expected inputs. The
    # lock set below re-validates every owner and artifact under the lock, so a
    # substitution between discovery and locking fails closed.
    inputs: list[_PublicationInput] = []
    directories: dict[tuple[str, str], Path] = {}
    owners: dict[tuple[str, str], Mapping[str, object]] = {}
    for unit in interface_units:
        directory = Path(unit)
        owner = _discover_source_unit_owner(directory)
        declared = owner["source"]
        assert isinstance(declared, dict)
        key = (str(declared["package"]), str(declared["path"]))
        if key in directories:
            raise _PublicationError("two interface units declare the same source")
        directories[key] = directory
        owners[key] = owner
        inputs.append(
            _PublicationInput(
                destination=directory,
                owner=owner,
                stable_validate=_validate_source_unit_header,
                recovery_validate=_validate_full_source_unit,
            )
        )

    with _publication_lock_set(
        inputs=inputs,
        outputs=[
            _PublicationOutput(
                destination=destination, recovery_validate=_validate_full_source_unit
            )
        ],
        filesystem=fs,
    ) as locks:
        # One stable snapshot of the source; the native compiler never reopens it.
        captured = _capture_source_file(source, source_root=source_root)
        relative = captured.path.relative_to(captured.source_root).as_posix()
        stem = captured.path.stem
        owner = _publication_owner_source_unit(package=package, path=relative)

        with tempfile.TemporaryDirectory(
            prefix="pycircuit-source-compile-"
        ) as scratch_name:
            scratch = Path(scratch_name)
            transport = scratch / f"{stem}.transport.mlir"
            transport.write_text(_emit_source_transport(captured), encoding="utf-8")

            header_paths: list[Path] = []
            views: dict[tuple[str, str], object] = {}
            for index, key in enumerate(sorted(directories)):
                # The owner recorded before locking is the one the lock set
                # validated against; reuse it rather than re-reading the receipt
                # inside the held lock.
                view = locks.snapshot(
                    directories[key],
                    functools.partial(
                        _read_header_view, owner=owners[key], filesystem=fs
                    ),
                )
                views[key] = view
                header_path = scratch / f"header_{index}.ac"
                header_path.write_text(view.interface, encoding="utf-8")  # type: ignore[attr-defined]
                header_paths.append(header_path)

            body_path = scratch / f"{stem}.ac.tmp"
            interface_path = scratch / f"{stem}.interface.ac.tmp"
            deps_path = scratch / "consumed.json"
            command = [
                str(compiler),
                "--capture",
                str(transport),
                "--package",
                package,
                "--path",
                relative,
                "--body-out",
                str(body_path),
                "--interface-out",
                str(interface_path),
                "--deps-out",
                str(deps_path),
            ]
            for header_path in header_paths:
                command.extend(("--header", str(header_path)))
            completed = subprocess.run(
                command, text=True, capture_output=True, check=False
            )
            if completed.returncode != 0:
                detail = completed.stderr.strip() or completed.stdout.strip()
                raise _PublicationError(
                    f"native source compiler rejected the source: {detail}"
                )
            if completed.stderr:
                sys.stderr.write(completed.stderr)
            if not body_path.is_file() or not interface_path.is_file():
                raise _PublicationError(
                    "native source compiler did not produce both artifacts"
                )

            prerequisites: list[Path] = [captured.path]
            for key in _read_consumed_dependencies(deps_path):
                directory = directories.get(key)
                if directory is None:
                    raise _PublicationError(
                        "native compiler consumed an interface unit that was not supplied"
                    )
                receipt = views[key].receipt  # type: ignore[attr-defined]
                prerequisites.append(directory / receipt.interface)
                prerequisites.append(directory / "unit.json")
            prerequisites.append(compiler)

            body_text = body_path.read_text(encoding="utf-8")
            interface_text = interface_path.read_text(encoding="utf-8")
            depfile_text = _depfile_text(destination / f"{stem}.ac", prerequisites)
            receipt_text = _receipt_text(
                package=package, source_path=relative, stem=stem
            )

            def build(stage: Path) -> None:
                (stage / f"{stem}.ac").write_text(body_text, encoding="utf-8")
                (stage / f"{stem}.interface.ac").write_text(
                    interface_text, encoding="utf-8"
                )
                (stage / f"{stem}.d").write_text(depfile_text, encoding="utf-8")
                (stage / "unit.json").write_text(receipt_text, encoding="utf-8")

            result = _publish_artifact(
                destination,
                owner=owner,
                build=build,
                validate=_validate_full_source_unit,
                replace=replace,
                cancelled=cancelled,
                filesystem=fs,
                locks=locks,
            )

    return _SourceCompileResult(
        destination=destination,
        owner=owner,
        body=f"{stem}.ac",
        interface=f"{stem}.interface.ac",
        depfile=f"{stem}.d",
        result=result,
    )

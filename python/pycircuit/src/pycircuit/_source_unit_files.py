"""Private source-unit receipt, file, and managed-read validation."""

from __future__ import annotations

import json
from collections.abc import Mapping
from dataclasses import dataclass
from pathlib import Path

from ._publication import (
    _Paths,
    _paths_for,
    _PublicationError,
    _PublicationFileSystem,
    _recover_publication,
    _validate_control,
    _validate_journal,
    _validate_owner,
)
from ._publication_fs import _PublicationFileSystemError
from ._source_capture import _read_stable_file_bytes


@dataclass(frozen=True, slots=True)
class _SourceUnitReceipt:
    package: str
    path: str
    body: str
    interface: str
    depfile: str


@dataclass(frozen=True, slots=True)
class _SourceUnitHeaderView:
    receipt: _SourceUnitReceipt
    interface: str


@dataclass(frozen=True, slots=True)
class _FullSourceUnit:
    receipt: _SourceUnitReceipt
    body: str
    interface: str
    depfile: str


def _reject_duplicate_keys(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key!r}")
        result[key] = value
    return result


def _read_text(path: Path, *, purpose: str) -> str:
    try:
        content = _read_stable_file_bytes(path).decode("utf-8")
    except (OSError, UnicodeDecodeError, ValueError) as error:
        raise _PublicationError(
            f"source-unit {purpose} is not stable UTF-8: {path}"
        ) from error
    if not content:
        raise _PublicationError(f"source-unit {purpose} is empty: {path}")
    return content


def _read_strict_json(path: Path, *, purpose: str) -> object:
    text = _read_text(path, purpose=purpose)
    try:
        return json.loads(text, object_pairs_hook=_reject_duplicate_keys)
    except (json.JSONDecodeError, ValueError) as error:
        raise _PublicationError(
            f"source-unit {purpose} is not strict JSON: {path}"
        ) from error


def _receipt(path: Path, owner: Mapping[str, object]) -> _SourceUnitReceipt:
    value = _read_strict_json(path / "unit.json", purpose="receipt")
    if type(value) is not dict or set(value) != {"kind", "source", "files"}:
        raise _PublicationError("source-unit receipt fields are not closed")
    if value["kind"] != "pycircuit-source-unit":
        raise _PublicationError("source-unit receipt kind is invalid")
    source = value["source"]
    files = value["files"]
    if type(source) is not dict or set(source) != {"package", "path"}:
        raise _PublicationError("source-unit receipt source fields are not closed")
    if type(files) is not dict or set(files) != {"body", "interface", "depfile"}:
        raise _PublicationError("source-unit receipt file fields are not closed")

    expected = _validate_owner(owner)
    if expected["kind"] != "source-unit" or source != expected["source"]:
        raise _PublicationError("source-unit receipt owner does not match")
    source_path = source["path"]
    package = source["package"]
    assert type(source_path) is str and type(package) is str
    stem = Path(source_path).stem
    expected_files = {
        "body": f"{stem}.ac",
        "interface": f"{stem}.interface.ac",
        "depfile": f"{stem}.d",
    }
    if files != expected_files:
        raise _PublicationError(
            "source-unit receipt file names do not match source stem"
        )
    return _SourceUnitReceipt(
        package=package,
        path=source_path,
        body=expected_files["body"],
        interface=expected_files["interface"],
        depfile=expected_files["depfile"],
    )


def _require_regular(path: Path, filesystem: _PublicationFileSystem) -> None:
    try:
        filesystem.require_kind(path, "file")
    except _PublicationFileSystemError as error:
        raise _PublicationError(
            f"source-unit file is missing or unsafe: {path}"
        ) from error


def _read_header_view(
    path: Path,
    owner: Mapping[str, object],
    filesystem: _PublicationFileSystem,
) -> _SourceUnitHeaderView:
    _require_regular(path / "unit.json", filesystem)
    receipt = _receipt(path, owner)
    interface_path = path / receipt.interface
    _require_regular(interface_path, filesystem)
    return _SourceUnitHeaderView(
        receipt=receipt,
        interface=_read_text(interface_path, purpose="interface"),
    )


def _read_full_source_unit(
    path: Path,
    owner: Mapping[str, object],
    filesystem: _PublicationFileSystem,
) -> _FullSourceUnit:
    try:
        filesystem.validate_plain_tree(path)
    except _PublicationFileSystemError as error:
        raise _PublicationError(f"source-unit tree is unsafe: {path}") from error
    receipt = _receipt(path, owner)
    expected_names = {
        "unit.json",
        receipt.body,
        receipt.interface,
        receipt.depfile,
    }
    actual_names = {entry.name for entry in path.iterdir()}
    if actual_names != expected_names:
        raise _PublicationError("source-unit directory file set is not closed")
    for name in expected_names:
        _require_regular(path / name, filesystem)
    return _FullSourceUnit(
        receipt=receipt,
        body=_read_text(path / receipt.body, purpose="body"),
        interface=_read_text(path / receipt.interface, purpose="interface"),
        depfile=_read_text(path / receipt.depfile, purpose="depfile"),
    )


def _validate_full_source_unit(path: Path, owner: Mapping[str, object]) -> None:
    """Publication callback validating a complete source unit."""

    _read_full_source_unit(path, owner, _PublicationFileSystem())


def _strict_control_metadata(paths: _Paths) -> dict[str, object] | None:
    marker = _read_strict_json(paths.owner, purpose="control owner")
    expected_marker = {
        "kind": "pycircuit-publication-control",
        "destination": paths.destination.name,
    }
    if marker != expected_marker:
        raise _PublicationError("source-unit publication control owner is invalid")
    if paths.journal.exists():
        journal = _read_strict_json(paths.journal, purpose="publication journal")
        return _validate_journal(journal, paths.destination.name)
    return None


def _load_managed_source_unit(
    destination: str | Path,
    *,
    owner: Mapping[str, object],
    full: bool,
) -> _SourceUnitHeaderView | _FullSourceUnit:
    expected_owner = _validate_owner(owner)
    if expected_owner["kind"] != "source-unit":
        raise _PublicationError("source-unit reader requires a source-unit owner")
    filesystem = _PublicationFileSystem()
    paths = _paths_for(destination, filesystem)
    if filesystem.kind(paths.control) != "directory":
        raise _PublicationError("unmanaged source-unit input is not accepted")
    filesystem.assert_no_symlink_chain(paths.control)
    _require_regular(paths.lock, filesystem)

    while True:
        needs_recovery = False
        with filesystem.lock(paths.lock, shared=True):
            journal = _strict_control_metadata(paths)
            _validate_control(paths, filesystem, cleanup_temporary=False)
            if journal is None:
                if (
                    filesystem.kind(paths.stage) is not None
                    or filesystem.kind(paths.previous) is not None
                ):
                    raise _PublicationError(
                        "source-unit transaction directories exist without journal"
                    )
                _validate_destination(paths.destination, filesystem)
                if full:
                    return _read_full_source_unit(
                        paths.destination, expected_owner, filesystem
                    )
                return _read_header_view(paths.destination, expected_owner, filesystem)
            if journal["owner"] != expected_owner:
                raise _PublicationError("published source-unit owner does not match")
            if journal["phase"] == "committed":
                if filesystem.kind(paths.stage) is not None:
                    raise _PublicationError(
                        "committed source-unit still has a stage directory"
                    )
                if (
                    not journal["had_previous"]
                    and filesystem.kind(paths.previous) is not None
                ):
                    raise _PublicationError(
                        "new committed source-unit has a previous directory"
                    )
                _validate_destination(paths.destination, filesystem)
                complete = _read_full_source_unit(
                    paths.destination, expected_owner, filesystem
                )
                if full:
                    return complete
                return _SourceUnitHeaderView(
                    receipt=complete.receipt, interface=complete.interface
                )
            needs_recovery = True
        if needs_recovery:
            _recover_publication(
                paths.destination,
                validate=_validate_full_source_unit,
                filesystem=filesystem,
            )


def _validate_destination(
    destination: Path, filesystem: _PublicationFileSystem
) -> None:
    if filesystem.kind(destination) != "directory":
        raise _PublicationError(
            "source-unit destination is missing or is not a directory"
        )
    try:
        filesystem.assert_no_symlink_chain(destination)
    except _PublicationFileSystemError as error:
        raise _PublicationError(
            "source-unit destination traverses a symlink"
        ) from error


def _load_source_unit_header(
    destination: str | Path, *, owner: Mapping[str, object]
) -> _SourceUnitHeaderView:
    result = _load_managed_source_unit(destination, owner=owner, full=False)
    assert isinstance(result, _SourceUnitHeaderView)
    return result


def _load_full_source_unit(
    destination: str | Path, *, owner: Mapping[str, object]
) -> _FullSourceUnit:
    result = _load_managed_source_unit(destination, owner=owner, full=True)
    assert isinstance(result, _FullSourceUnit)
    return result


def _validate_source_unit_header(path: Path, owner: Mapping[str, object]) -> None:
    """Header-only publication callback: receipt plus interface only.

    Normal parent compilation must accept a managed provider without reading its
    body or depfile; recovery still uses the full validator.
    """

    _read_header_view(path, owner, _PublicationFileSystem())


def _discover_source_unit_owner(destination: str | Path) -> dict[str, object]:
    """Read a managed unit's declared owner without granting it authority.

    The result is only a candidate: the caller must present it as the expected
    owner to a lock set that re-validates it under the lock before use.
    """

    value = _read_strict_json(Path(destination) / "unit.json", purpose="receipt")
    if type(value) is not dict or set(value) != {"kind", "source", "files"}:
        raise _PublicationError("source-unit receipt fields are not closed")
    if value["kind"] != "pycircuit-source-unit":
        raise _PublicationError("source-unit receipt kind is invalid")
    source = value["source"]
    if type(source) is not dict or set(source) != {"package", "path"}:
        raise _PublicationError("source-unit receipt source fields are not closed")
    package = source["package"]
    path = source["path"]
    if type(package) is not str or type(path) is not str:
        raise _PublicationError("source-unit receipt source is not textual")
    return {"kind": "source-unit", "source": {"package": package, "path": path}}


__all__ = [
    "_FullSourceUnit",
    "_SourceUnitHeaderView",
    "_SourceUnitReceipt",
    "_discover_source_unit_owner",
    "_load_full_source_unit",
    "_load_source_unit_header",
    "_read_header_view",
    "_validate_full_source_unit",
    "_validate_source_unit_header",
]

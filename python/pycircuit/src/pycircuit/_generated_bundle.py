"""Private receipt/file validation for empty-static-argument generated artifacts.

This reader checks file management, not generated-code semantics or ABI
completeness. Production emission must bind inventory to the native result.
"""

from __future__ import annotations

import json
import math
import os
from collections.abc import Mapping
from dataclasses import dataclass
from pathlib import Path, PurePosixPath, PureWindowsPath

from ._publication import (
    _normalized_path_parts,
    _PublicationError,
    _PublicationFileSystem,
    _read_published,
    _validate_owner,
)
from ._publication_fs import _PublicationFileSystemError
from ._source_capture import _read_stable_file_bytes

_GENERATED_ROLES = {
    "header",
    "source",
    "cmake",
    "rtl",
    "runtime-glue",
    "source-map",
}


@dataclass(frozen=True, slots=True)
class _GeneratedBundleSnapshot:
    receipt: bytes
    files: tuple[tuple[str, bytes], ...]


def _reject_duplicate_keys(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key!r}")
        result[key] = value
    return result


def _reject_nonfinite(value: str) -> object:
    raise ValueError(f"non-finite JSON number: {value}")


def _parse_finite_float(value: str) -> float:
    parsed = float(value)
    if not math.isfinite(parsed):
        raise ValueError(f"non-finite JSON number: {value}")
    return parsed


def _source_owner(value: object, *, purpose: str) -> dict[str, str]:
    if type(value) is not dict or set(value) != {"package", "path"}:
        raise _PublicationError(f"generated {purpose} owner fields are not closed")
    owner = _validate_owner({"kind": "source-unit", "source": value})
    source = owner["source"]
    package = source["package"]
    path = source["path"]
    assert type(package) is str and type(path) is str
    return {"package": package, "path": path}


def _relative_file_path(value: object) -> tuple[str, tuple[str, ...], bytes]:
    if type(value) is not str or not value or "\x00" in value or "\\" in value:
        raise _PublicationError("generated file path is invalid")
    components = tuple(value.split("/"))
    if any(component in {"", ".", ".."} for component in components):
        raise _PublicationError("generated file path is not canonical POSIX")
    if any(":" in component for component in components):
        raise _PublicationError("generated file path contains a drive or stream marker")
    posix = PurePosixPath(value)
    windows = PureWindowsPath(value)
    if posix.is_absolute() or windows.is_absolute() or windows.drive:
        raise _PublicationError("generated file path must be relative")
    try:
        encoded = value.encode("utf-8")
    except UnicodeEncodeError as error:
        raise _PublicationError("generated file path is not valid Unicode") from error
    return value, components, encoded


def _generated_receipt(
    receipt: bytes, expected_owner: Mapping[str, object]
) -> list[tuple[str, str]]:
    try:
        text = receipt.decode("utf-8")
        value = json.loads(
            text,
            object_pairs_hook=_reject_duplicate_keys,
            parse_constant=_reject_nonfinite,
            parse_float=_parse_finite_float,
        )
    except (
        UnicodeDecodeError,
        json.JSONDecodeError,
        RecursionError,
        ValueError,
    ) as error:
        raise _PublicationError("generated.json is not strict UTF-8 JSON") from error

    if type(value) is not dict or set(value) != {
        "kind",
        "target",
        "entry",
        "entry_source",
        "files",
        "source_groups",
    }:
        raise _PublicationError("generated.json fields are not closed")
    if value["kind"] != "pycircuit-generated":
        raise _PublicationError("generated.json kind is invalid")
    if type(value["target"]) is not str or value["target"] != expected_owner["target"]:
        raise _PublicationError("generated.json target does not match its owner")

    entry = value["entry"]
    if type(entry) is not dict or set(entry) != {"definition", "arguments"}:
        raise _PublicationError("generated.json entry fields are not closed")
    if (
        type(entry["definition"]) is not str
        or entry["definition"] != expected_owner["definition"]
    ):
        raise _PublicationError("generated.json entry does not match its owner")
    if type(entry["arguments"]) is not list:
        raise _PublicationError("generated.json entry arguments must be an array")
    if entry["arguments"]:
        raise _PublicationError(
            "generated static arguments are not implemented for this reader"
        )

    entry_source = _source_owner(value["entry_source"], purpose="entry source")
    if entry_source != expected_owner["source"]:
        raise _PublicationError("generated.json entry source does not match its owner")

    raw_files = value["files"]
    if type(raw_files) is not list:
        raise _PublicationError("generated.json files must be an array")
    files: list[tuple[str, str]] = []
    declared: set[str] = set()
    previous_path: bytes | None = None
    path_spellings: dict[tuple[str, ...], tuple[str, ...]] = {}
    for index, raw in enumerate(raw_files):
        if type(raw) is not dict or set(raw) != {"path", "role"}:
            raise _PublicationError(f"generated.json files[{index}] is not closed")
        path, components, encoded = _relative_file_path(raw["path"])
        role = raw["role"]
        if type(role) is not str or role not in _GENERATED_ROLES:
            raise _PublicationError(f"generated.json files[{index}] role is invalid")
        if path == "generated.json":
            raise _PublicationError("generated.json cannot list itself")
        if previous_path is not None and encoded <= previous_path:
            raise _PublicationError(
                "generated.json files must be unique and sorted by UTF-8 path"
            )
        previous_path = encoded
        if path in declared:
            raise _PublicationError("generated.json repeats a file path")
        declared.add(path)
        files.append((path, role))

        for length in range(1, len(components) + 1):
            prefix = components[:length]
            normalized = _normalized_path_parts(Path(*prefix))
            previous = path_spellings.get(normalized)
            if previous is not None and previous != prefix:
                raise _PublicationError(
                    "generated paths collide under filesystem case folding"
                )
            path_spellings[normalized] = prefix

    # Reserve the receipt path too: no emitted file or directory may alias it.
    reserved_receipt = ("generated.json",)
    normalized_receipt = _normalized_path_parts(Path(*reserved_receipt))
    previous = path_spellings.get(normalized_receipt)
    if previous is not None and previous != reserved_receipt:
        raise _PublicationError(
            "generated path collides with generated.json under filesystem case folding"
        )
    path_spellings[normalized_receipt] = reserved_receipt

    raw_groups = value["source_groups"]
    if type(raw_groups) is not list:
        raise _PublicationError("generated.json source_groups must be an array")
    owners: set[tuple[str, str]] = set()
    grouped_files: set[str] = set()
    for index, raw in enumerate(raw_groups):
        if type(raw) is not dict or set(raw) != {"source", "files"}:
            raise _PublicationError(
                f"generated.json source_groups[{index}] is not closed"
            )
        source = _source_owner(raw["source"], purpose="source group")
        source_key = (source["package"], source["path"])
        if source_key in owners:
            raise _PublicationError("generated.json repeats a source group owner")
        owners.add(source_key)
        raw_group_files = raw["files"]
        if type(raw_group_files) is not list:
            raise _PublicationError(
                f"generated.json source_groups[{index}].files must be an array"
            )
        local_files: set[str] = set()
        for raw_path in raw_group_files:
            path, _components, _encoded = _relative_file_path(raw_path)
            if path not in declared:
                raise _PublicationError(f"source group file is not declared: {path}")
            if path in local_files or path in grouped_files:
                raise _PublicationError(
                    f"generated file belongs to multiple source-group entries: {path}"
                )
            local_files.add(path)
            grouped_files.add(path)
    return files


def _read_regular_bytes(path: Path, filesystem: _PublicationFileSystem) -> bytes:
    try:
        filesystem.require_kind(path, "file")
        return _read_stable_file_bytes(path)
    except (OSError, ValueError, _PublicationFileSystemError) as error:
        raise _PublicationError(
            f"generated bundle file is missing or unstable: {path}"
        ) from error


def _read_generated_bundle(
    path: Path,
    owner: Mapping[str, object],
    filesystem: _PublicationFileSystem,
) -> _GeneratedBundleSnapshot:
    expected_owner = _validate_owner(owner)
    if expected_owner["kind"] != "generated":
        raise _PublicationError("generated bundle reader requires a generated owner")
    root = Path(path)
    try:
        root = filesystem.absolute(path)
        filesystem.assert_no_symlink_chain(root)
        filesystem.validate_plain_tree(root)
    except (OSError, ValueError, _PublicationFileSystemError) as error:
        raise _PublicationError(f"generated bundle tree is unsafe: {root}") from error

    receipt = _read_regular_bytes(root / "generated.json", filesystem)
    files = _generated_receipt(receipt, expected_owner)

    expected_files = {"generated.json", *(path for path, _role in files)}
    expected_directories: set[str] = set()
    for relative in expected_files:
        parts = relative.split("/")
        for length in range(1, len(parts)):
            expected_directories.add("/".join(parts[:length]))
    if expected_files & expected_directories:
        raise _PublicationError("generated bundle path is both file and directory")

    actual_files: set[str] = set()
    actual_directories: set[str] = set()
    try:
        for directory, names, file_names in os.walk(root, followlinks=False):
            directory_path = Path(directory)
            for name in names:
                actual_directories.add(
                    (directory_path / name).relative_to(root).as_posix()
                )
            for name in file_names:
                actual_files.add((directory_path / name).relative_to(root).as_posix())
    except OSError as error:
        raise _PublicationError(
            f"generated bundle tree changed while reading: {root}"
        ) from error
    if actual_files != expected_files or actual_directories != expected_directories:
        raise _PublicationError("generated bundle directory file set is not closed")

    snapshot_files = tuple(
        (relative, _read_regular_bytes(root / relative, filesystem))
        for relative, _role in files
    )
    return _GeneratedBundleSnapshot(receipt=receipt, files=snapshot_files)


def _validate_generated_bundle(path: Path, owner: Mapping[str, object]) -> None:
    _read_generated_bundle(path, owner, _PublicationFileSystem())


def _load_generated_bundle(
    destination: str | Path,
    *,
    owner: Mapping[str, object],
    filesystem: _PublicationFileSystem | None = None,
) -> _GeneratedBundleSnapshot:
    expected_owner = _validate_owner(owner)
    if expected_owner["kind"] != "generated":
        raise _PublicationError("generated bundle reader requires a generated owner")
    fs = filesystem or _PublicationFileSystem()
    return _read_published(
        destination,
        owner=expected_owner,
        stable_validate=_validate_generated_bundle,
        recovery_validate=_validate_generated_bundle,
        read=lambda path: _read_generated_bundle(path, expected_owner, fs),
        filesystem=fs,
    )

"""Bounded saved-anchor transport. ACIR meaning is verified only by native code."""

from __future__ import annotations

import hashlib
import json
import os
import re
import stat
import tempfile
from collections.abc import Mapping
from pathlib import Path, PurePosixPath

from ._publication import (
    _normalized_path_parts,
    _paths_for,
    _PublicationError,
    _PublicationFileSystem,
    _read_journal,
    _require_initialized_control,
    _validate_control,
    _validate_owner,
    _validate_qualified_symbol,
)

_MANIFEST_LIMIT = 4 * 1024 * 1024
_PAYLOAD_LIMIT = 16 * 1024 * 1024
_TOTAL_LIMIT = 64 * 1024 * 1024
_FILE_LIMIT = 4096
_DIRECTORY_LIMIT = 8192
_DEPTH_LIMIT = 32
_PATH_LIMIT = 1024
_HEX = r"[0-9a-f]{64}"
_ENTRY_BOUNDARY_SYMBOL = '@"pycircuit.__link_entry__"'
_PATHS = {
    "graph": r"graph\.ac",
    "spec": rf"definitions/{_HEX}/spec\.ac",
    "body": rf"definitions/{_HEX}/body\.ac",
    "source-interface": rf"sources/{_HEX}\.interface\.ac",
}


def _canonical(value: object) -> bytes:
    return json.dumps(
        value,
        ensure_ascii=False,
        sort_keys=True,
        separators=(",", ":"),
        allow_nan=False,
    ).encode("utf-8")


def _digest(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def _object(value: object, keys: set[str], name: str) -> dict:
    if type(value) is not dict or set(value) != keys:
        raise _PublicationError(f"anchor {name} fields are not closed")
    return value


def _array(value: object, name: str) -> list:
    if type(value) is not list:
        raise _PublicationError(f"anchor {name} must be an array")
    return value


def _text(value: object, name: str) -> str:
    if type(value) is not str:
        raise _PublicationError(f"anchor {name} must be textual")
    return value


def _hash(value: object) -> str:
    if type(value) is not str or re.fullmatch(_HEX, value) is None:
        raise _PublicationError("anchor identity must be lowercase SHA256")
    return value


def _source(value: object) -> dict:
    source = _object(value, {"package", "path"}, "source")
    _validate_owner({"kind": "source-unit", "source": source})
    return source


def _symbol(value: object) -> str:
    symbol = _text(value, "definition")
    _validate_qualified_symbol(symbol)
    return symbol


def _unique_object(pairs: list[tuple[str, object]]) -> dict:
    result = {}
    for name, value in pairs:
        if name in result:
            raise _PublicationError("anchor JSON contains a duplicate key")
        result[name] = value
    return result


def _state(info: os.stat_result) -> tuple:
    return (
        info.st_dev,
        info.st_ino,
        info.st_mode,
        info.st_size,
        info.st_mtime_ns,
        info.st_ctime_ns,
    )


def _read_bounded(path: Path, limit: int, fs: _PublicationFileSystem) -> bytes:
    fs.assert_no_symlink_chain(path)
    descriptor = fs._open_regular(path)
    try:
        before = os.fstat(descriptor)
        if before.st_size > limit:
            raise _PublicationError(
                f"anchor file exceeds format byte limit: {path.name}"
            )
        chunks = []
        size = 0
        while True:
            chunk = os.read(descriptor, min(1024 * 1024, limit - size + 1))
            if not chunk:
                break
            size += len(chunk)
            if size > limit:
                raise _PublicationError("anchor file grew beyond format byte limit")
            chunks.append(chunk)
        after = os.fstat(descriptor)
        current = path.stat(follow_symlinks=False)
        if _state(before) != _state(after) or _state(after) != _state(current):
            raise _PublicationError("anchor file changed while capturing its snapshot")
        if size != before.st_size:
            raise _PublicationError("anchor file size changed while capturing")
        return b"".join(chunks)
    finally:
        os.close(descriptor)


def _manifest(path: Path, fs: _PublicationFileSystem) -> tuple[dict, bytes]:
    raw = _read_bounded(path / "anchor.json", _MANIFEST_LIMIT, fs)
    try:
        value = json.loads(
            raw.decode("utf-8"),
            object_pairs_hook=_unique_object,
            parse_constant=lambda _: (_ for _ in ()).throw(
                ValueError("nonfinite JSON")
            ),
        )
        canonical = _canonical(value)
    except (ValueError, UnicodeError, RecursionError) as error:
        raise _PublicationError(
            "anchor manifest is not bounded canonical UTF-8 JSON"
        ) from error
    if raw != canonical + b"\n":
        raise _PublicationError(
            "anchor manifest requires canonical JSON and exactly one LF"
        )
    _validate_manifest(value)
    return value, raw


def _validate_manifest(value: object) -> None:
    manifest = _object(
        value,
        {
            "kind",
            "schema",
            "routed_contract",
            "owner",
            "root",
            "configuration",
            "configuration_identity",
            "content_identity",
            "files",
            "definitions",
            "sources",
            "producer",
        },
        "manifest",
    )
    if (
        manifest["kind"] != "pycircuit-anchor"
        or type(manifest["schema"]) is not int
        or type(manifest["routed_contract"]) is not int
        or (manifest["schema"], manifest["routed_contract"]) not in ((1, 2), (2, 3))
    ):
        raise _PublicationError(
            "unsupported anchor format/contract; regenerate from source units"
        )
    entry_boundary = manifest["schema"] == 2
    owner = _validate_owner(manifest["owner"])
    if owner["kind"] != "anchor" or _symbol(manifest["root"]) != owner["definition"]:
        raise _PublicationError("anchor root and owner disagree")
    producer = _object(
        manifest["producer"], {"tool", "version", "pipeline"}, "producer"
    )
    for entry in producer.values():
        _text(entry, "producer")
    files = {}
    folded = set()
    inventory = _array(manifest["files"], "files")
    if len(inventory) > _FILE_LIMIT:
        raise _PublicationError("anchor payload file count exceeds format limit")
    for row in inventory:
        row = _object(row, {"path", "role", "sha256"}, "file")
        path, role = _text(row["path"], "inventory path"), row["role"]
        if (
            type(role) is not str
            or role not in _PATHS
            or re.fullmatch(_PATHS[role], path) is None
            or len(path.encode("utf-8")) > _PATH_LIMIT
        ):
            raise _PublicationError("anchor inventory path or role is invalid")
        normalized = _normalized_path_parts(Path(path))
        if path in files or normalized in folded:
            raise _PublicationError("anchor inventory paths collide")
        files[path] = row
        folded.add(normalized)
        _hash(row["sha256"])
    if list(files) != sorted(files):
        raise _PublicationError("anchor payload inventory must be path-sorted")
    if files.get("graph.ac", {}).get("role") != "graph":
        raise _PublicationError("anchor inventory requires graph.ac")
    referenced = {"graph.ac"}
    definitions = {}
    variants = set()
    definition_order = []
    for row in _array(manifest["definitions"], "definitions"):
        row = _object(
            row,
            {
                "definition",
                "source",
                "variant_id",
                "spec",
                "body",
                "spec_sha256",
                "body_sha256",
                "dependencies",
            },
            "definition",
        )
        name = _symbol(row["definition"])
        source = _source(row["source"])
        variant = _hash(row["variant_id"])
        if name in definitions or variant in variants:
            raise _PublicationError("anchor definitions or variant identities collide")
        for role in ("spec", "body"):
            path = f"definitions/{variant}/{role}.ac"
            if (
                row[role] != path
                or path not in files
                or files[path]["role"] != role
                or _hash(row[f"{role}_sha256"]) != files[path]["sha256"]
            ):
                raise _PublicationError(
                    "anchor definition inventory reference disagrees"
                )
            referenced.add(path)
        expected = _digest(
            _canonical(
                {
                    "source": source,
                    "definition": name,
                    "spec_sha256": row["spec_sha256"],
                    "body_sha256": row["body_sha256"],
                }
            )
        )
        if expected != variant:
            raise _PublicationError("anchor variant identity disagrees")
        dependencies = [
            _symbol(item) for item in _array(row["dependencies"], "dependencies")
        ]
        if dependencies != sorted(set(dependencies)):
            raise _PublicationError("anchor dependencies must be sorted and unique")
        definitions[name] = row
        definition_order.append(name)
        variants.add(variant)
    if definition_order != sorted(definition_order):
        raise _PublicationError("anchor definitions must be sorted")
    root = definitions.get(manifest["root"])
    if root is None or root["source"] != owner["source"]:
        raise _PublicationError(
            "anchor root definition/source is missing or mismatched"
        )
    sources = {}
    source_order = []
    for row in _array(manifest["sources"], "sources"):
        row = _object(
            row,
            {"source", "owner_id", "interface", "body_sha256", "interface_sha256"},
            "provenance",
        )
        source = _source(row["source"])
        key = (source["package"], source["path"])
        identity = _hash(row["owner_id"])
        path = f"sources/{identity}.interface.ac"
        if key in sources or identity != _digest(_canonical(source)):
            raise _PublicationError(
                "anchor source owner identity collides or disagrees"
            )
        if (
            row["interface"] != path
            or path not in files
            or files[path]["role"] != "source-interface"
            or _hash(row["interface_sha256"]) != files[path]["sha256"]
        ):
            raise _PublicationError("anchor interface inventory reference disagrees")
        _hash(row["body_sha256"])
        sources[key] = row
        source_order.append(_canonical(source))
        referenced.add(path)
    if source_order != sorted(source_order):
        raise _PublicationError("anchor source owners must be sorted")
    if any(
        (row["source"]["package"], row["source"]["path"]) not in sources
        for row in definitions.values()
    ):
        raise _PublicationError("anchor definition has no retained source provenance")
    if referenced != set(files):
        raise _PublicationError("anchor inventory contains unreferenced payload files")
    configuration = _object(
        manifest["configuration"],
        {
            "root",
            "integer_arguments",
            "type_arguments",
            "domain",
            "reset_semantics",
            "graph_sha256",
            "definitions",
        }
        | ({"implementation_root"} if entry_boundary else set()),
        "configuration",
    )
    config_root = _object(
        configuration["root"],
        {"source", "definition", "variant_id"},
        "configuration root",
    )
    if config_root != {
        "source": owner["source"],
        "definition": manifest["root"],
        "variant_id": root["variant_id"],
    }:
        raise _PublicationError("anchor configuration root disagrees")
    if entry_boundary:
        implementation = _object(
            configuration["implementation_root"],
            {"source", "definition", "variant_id"},
            "implementation root",
        )
        adapter = definitions.get(_ENTRY_BOUNDARY_SYMBOL)
        if (
            adapter is None
            or manifest["root"] == _ENTRY_BOUNDARY_SYMBOL
            or adapter["source"] != owner["source"]
            or implementation
            != {
                "source": owner["source"],
                "definition": _ENTRY_BOUNDARY_SYMBOL,
                "variant_id": adapter["variant_id"],
            }
        ):
            raise _PublicationError("anchor implementation root disagrees")
    names = set()
    for argument in _array(configuration["integer_arguments"], "integer arguments"):
        argument = _object(argument, {"name", "value"}, "integer argument")
        name = _text(argument["name"], "integer argument name")
        number = _text(argument["value"], "integer argument value")
        if (
            not name
            or name in names
            or re.fullmatch(r"0|-?[1-9][0-9]*", number) is None
        ):
            raise _PublicationError(
                "anchor integer arguments must be unique and canonical"
            )
        names.add(name)
    if configuration["type_arguments"] != []:
        raise _PublicationError("anchor public root type arguments must be empty")
    _text(configuration["domain"], "domain")
    if configuration["reset_semantics"] not in ("strict", "if_one"):
        raise _PublicationError("anchor reset semantics is invalid")
    if _hash(configuration["graph_sha256"]) != files["graph.ac"]["sha256"]:
        raise _PublicationError("anchor configuration graph digest disagrees")
    expected_definitions = [
        {
            key: row[key]
            for key in ("definition", "variant_id", "spec_sha256", "body_sha256")
        }
        for row in definitions.values()
    ]
    if configuration["definitions"] != expected_definitions:
        raise _PublicationError("anchor configuration definition identities disagree")
    if _hash(manifest["configuration_identity"]) != _digest(_canonical(configuration)):
        raise _PublicationError("anchor configuration identity disagrees")
    content = {key: item for key, item in manifest.items() if key != "content_identity"}
    if _hash(manifest["content_identity"]) != _digest(_canonical(content)):
        raise _PublicationError("anchor content identity disagrees")


def _capture_anchor(source: Path, destination: Path | None = None) -> dict:
    """Validate/copy one consistent plain tree before any native parsing."""
    fs = _PublicationFileSystem()
    fs.assert_no_symlink_chain(source)
    fs.require_kind(source, "directory")
    manifest, raw = _manifest(source, fs)
    expected_files = {row["path"]: row for row in manifest["files"]}
    expected_files["anchor.json"] = {"sha256": _digest(raw)}
    expected_directories = set()
    for name in expected_files:
        expected_directories.update(
            str(parent) for parent in PurePosixPath(name).parents if str(parent) != "."
        )
    before = {}
    actual_files = set()
    actual_directories = set()
    pending = [(source, "", 0)]
    total = 0
    while pending:
        directory, prefix, depth = pending.pop()
        info = directory.stat(follow_symlinks=False)
        if not stat.S_ISDIR(info.st_mode) or fs._is_reparse_point(info):
            raise _PublicationError("anchor directory changed or became unsafe")
        before[directory] = _state(info)
        with os.scandir(directory) as entries:
            for entry in entries:
                relative = f"{prefix}/{entry.name}" if prefix else entry.name
                if (
                    len(relative.encode("utf-8")) > _PATH_LIMIT
                    or depth + 1 > _DEPTH_LIMIT
                ):
                    raise _PublicationError("anchor tree exceeds path/depth limits")
                info = entry.stat(follow_symlinks=False)
                if entry.is_symlink() or fs._is_reparse_point(info):
                    raise _PublicationError(
                        "anchor tree contains a symlink/reparse point"
                    )
                if stat.S_ISDIR(info.st_mode):
                    actual_directories.add(relative)
                    if (
                        len(actual_directories) > _DIRECTORY_LIMIT
                        or relative not in expected_directories
                    ):
                        raise _PublicationError(
                            "anchor directory inventory is not closed"
                        )
                    pending.append((Path(entry.path), relative, depth + 1))
                elif stat.S_ISREG(info.st_mode):
                    actual_files.add(relative)
                    limit = (
                        _MANIFEST_LIMIT if relative == "anchor.json" else _PAYLOAD_LIMIT
                    )
                    if (
                        relative not in expected_files
                        or len(actual_files) > _FILE_LIMIT + 1
                        or info.st_size > limit
                    ):
                        raise _PublicationError(
                            "anchor file inventory or size exceeds format limits"
                        )
                    if relative != "anchor.json":
                        total += info.st_size
                        if total > _TOTAL_LIMIT:
                            raise _PublicationError(
                                "anchor aggregate payload exceeds format byte limit"
                            )
                else:
                    raise _PublicationError("anchor tree contains a nonordinary file")
    if (
        actual_files != set(expected_files)
        or actual_directories != expected_directories
    ):
        raise _PublicationError(
            "anchor tree differs from its exact file/directory inventory"
        )
    if destination is not None:
        destination.mkdir(parents=True, exist_ok=True)
    captured = 0
    for name, row in expected_files.items():
        limit = (
            _MANIFEST_LIMIT
            if name == "anchor.json"
            else min(_PAYLOAD_LIMIT, _TOTAL_LIMIT - captured)
        )
        data = _read_bounded(source / name, limit, fs)
        if name != "anchor.json":
            captured += len(data)
        if _digest(data) != row["sha256"]:
            raise _PublicationError(f"anchor payload digest disagrees: {name}")
        if destination is not None:
            target = destination / name
            target.parent.mkdir(parents=True, exist_ok=True)
            with target.open("xb") as stream:
                stream.write(data)
    for directory, identity in before.items():
        fs.assert_no_symlink_chain(directory)
        if _state(directory.stat(follow_symlinks=False)) != identity:
            raise _PublicationError("anchor directory changed while capturing")
    return manifest


def _discover_anchor_owner(source: Path) -> tuple[dict, bool]:
    """Controls, when present, are authoritative operational state and fail closed."""
    fs = _PublicationFileSystem()
    paths = _paths_for(source, fs)
    if fs.kind(paths.control) is None:
        return _manifest(paths.destination, fs)[0]["owner"], False
    _require_initialized_control(paths, fs)
    with fs.lock(paths.lock, shared=True):
        _validate_control(paths, fs, cleanup_temporary=False)
        journal = _read_journal(paths, fs, cleanup_temporary=False)
        if journal is not None:
            owner = _validate_owner(journal["owner"])
            if owner["kind"] != "anchor":
                raise _PublicationError("managed anchor has a different artifact owner")
            return owner, True
        return _manifest(paths.destination, fs)[0]["owner"], True


def _validate_anchor(path: Path, owner: Mapping[str, object]) -> None:
    from . import _native_verify

    with tempfile.TemporaryDirectory(prefix="pycircuit-anchor-verify-") as scratch:
        snapshot = Path(scratch).resolve() / "anchor"
        manifest = _capture_anchor(path, snapshot)
        if manifest["owner"] != dict(owner) or _native_verify.verify_anchor_owner(
            snapshot
        ) != dict(owner):
            raise _PublicationError(
                "anchor verified owner does not match publication owner"
            )

"""Validate file-management consistency of approved provenance inventories."""

from __future__ import annotations

import json
import subprocess
from collections.abc import Mapping
from pathlib import Path

from ._generated_bundle import _read_generated_bundle, _reject_duplicate_keys
from ._native_verify import native_helper
from ._publication import _PublicationError
from ._publication_fs import _PublicationFileSystem


def _validate_emitted_bundle(path: Path, owner: Mapping[str, object]) -> None:
    snapshot = _read_generated_bundle(path, owner, _PublicationFileSystem())
    receipt = json.loads(snapshot.receipt)
    contents = dict(snapshot.files)
    roles = {row["path"]: row["role"] for row in receipt["files"]}
    expected_maps = {name for name, role in roles.items() if role == "source-map"}
    found_maps = set()
    for group in receipt["source_groups"]:
        maps = [name for name in group["files"] if roles[name] == "source-map"]
        if len(maps) != 1:
            raise _PublicationError("source group requires exactly one provenance map")
        name = maps[0]
        found_maps.add(name)
        try:
            value = json.loads(contents[name], object_pairs_hook=_reject_duplicate_keys)
        except (UnicodeError, ValueError) as error:
            raise _PublicationError("source map is not strict UTF-8 JSON") from error
        if type(value) is not dict or set(value) != {
            "kind",
            "source",
            "generated_files",
            "origins",
        }:
            raise _PublicationError("source map fields are not closed")
        if (
            value["kind"] != "pycircuit-source-map"
            or value["source"] != group["source"]
        ):
            raise _PublicationError("source map owner or kind does not match")
        expected = sorted(
            (item for item in group["files"] if item != name),
            key=lambda item: item.encode("utf-8"),
        )
        if value["generated_files"] != expected:
            raise _PublicationError("source map file membership does not match")
        # MLIR is the authority for canonical Occurrence/Location attributes.
        completed = subprocess.run(
            [str(native_helper("emit")), "--verify-source-map", str(path / name)],
            capture_output=True,
            text=True,
            check=False,
        )
        if completed.returncode:
            detail = " ".join((completed.stderr or completed.stdout).split())
            raise _PublicationError(f"source map failed native verification: {detail}")
        try:
            native_path = json.loads(
                completed.stdout, object_pairs_hook=_reject_duplicate_keys
            )
        except ValueError as error:
            raise _PublicationError(
                "native source-map path report is invalid"
            ) from error
        if (
            type(native_path) is not dict
            or set(native_path) != {"path"}
            or native_path["path"] != name
        ):
            raise _PublicationError("source map path does not match its SourceOwner")
    if found_maps != expected_maps or not found_maps:
        raise _PublicationError("source map inventory is incomplete")

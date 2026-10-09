"""Independent structural checks for the approved source-map inventory."""

from __future__ import annotations

import json
import os
import subprocess
from pathlib import Path
from typing import Any

import pytest
from pycircuit._native_verify import native_helper
from pycircuit._publication import _publication_owner_generated, _PublicationError
from pycircuit._source_map import _validate_emitted_bundle

# This lives beside other validator-focused cases, but it deliberately invokes
# the native MLIR attribute verifier and therefore belongs to the system gate.
pytestmark = pytest.mark.system

OWNER = _publication_owner_generated(
    package="demo", path="root.py", definition='@"demo.root.Root"', target="cpp"
)
SOURCE = {"package": "demo", "path": "root.py"}
MAP_PATH = "sources/demo/root.source-map.json"
GENERATED_PATH = "sources/demo/root.hpp"


def _map(
    *, generated: list[str] | None = None, origins: list[dict[str, str]] | None = None
):
    return {
        "kind": "pycircuit-source-map",
        "source": SOURCE,
        "generated_files": [GENERATED_PATH] if generated is None else generated,
        "origins": [] if origins is None else origins,
    }


def _receipt(
    *,
    group_files: list[str] | None = None,
    listed_files: list[dict[str, str]] | None = None,
):
    names = [GENERATED_PATH, MAP_PATH] if group_files is None else group_files
    files = (
        [
            {"path": GENERATED_PATH, "role": "header"},
            {"path": MAP_PATH, "role": "source-map"},
        ]
        if listed_files is None
        else listed_files
    )
    return {
        "kind": "pycircuit-generated",
        "target": "cpp",
        "entry": {"definition": '@"demo.root.Root"', "arguments": []},
        "entry_source": SOURCE,
        "files": sorted(files, key=lambda item: item["path"].encode("utf-8")),
        "source_groups": [{"source": SOURCE, "files": names}],
    }


def _bundle(
    root: Path,
    *,
    value: Any | None = None,
    raw_map: bytes | None = None,
    receipt: dict[str, Any] | None = None,
    disk_files: dict[str, bytes] | None = None,
) -> Path:
    root.mkdir(parents=True, exist_ok=True)
    files = (
        {GENERATED_PATH: b"generated header\n"} if disk_files is None else disk_files
    )
    for relative, content in files.items():
        path = root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content)
    map_file = root / MAP_PATH
    map_file.parent.mkdir(parents=True, exist_ok=True)
    if raw_map is not None:
        map_file.write_bytes(raw_map)
    else:
        map_file.write_text(
            json.dumps(_map() if value is None else value, separators=(",", ":")),
            encoding="utf-8",
        )
    (root / "generated.json").write_text(
        json.dumps(_receipt() if receipt is None else receipt, separators=(",", ":")),
        encoding="utf-8",
    )
    return root


def _assert_bundle_rejected(path: Path) -> None:
    with pytest.raises(_PublicationError):
        _validate_emitted_bundle(path, OWNER)


def _native_map_check(path: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(native_helper("emit")), "--verify-source-map", str(path)],
        text=True,
        capture_output=True,
        check=False,
        timeout=30,
        env=os.environ.copy(),
    )


def test_valid_empty_inventory_is_registered_once_and_matches_its_group(
    tmp_path: Path,
) -> None:
    bundle = _bundle(tmp_path / "bundle")

    _validate_emitted_bundle(bundle, OWNER)


@pytest.mark.parametrize(
    "raw",
    [
        b"\xff\xfe",  # invalid UTF-8
        b'{"kind":"pycircuit-source-map","kind":"pycircuit-source-map"}',
    ],
    ids=["invalid-utf8", "duplicate-json-key"],
)
def test_map_file_must_be_strict_utf8_json_with_unique_keys(
    tmp_path: Path, raw: bytes
) -> None:
    bundle = _bundle(tmp_path / "bundle", raw_map=raw)

    _assert_bundle_rejected(bundle)


@pytest.mark.parametrize(
    "mutate",
    [
        lambda value: value.pop("origins"),
        lambda value: value.update(extra=True),
        lambda value: value.update(kind="other"),
        lambda value: value.update(source={"package": "demo", "path": "../root.py"}),
        lambda value: value.update(generated_files=[GENERATED_PATH, GENERATED_PATH]),
        lambda value: value.update(generated_files=["../outside.hpp"]),
        lambda value: value.update(
            origins=[{"operation": "x", "origin": "x", "location": "x"}]
        ),
    ],
    ids=[
        "missing-field",
        "unknown-field",
        "wrong-kind",
        "invalid-owner",
        "duplicate-file",
        "unsafe-file",
        "invalid-origin",
    ],
)
def test_native_validator_rejects_malformed_standalone_maps(
    tmp_path: Path, mutate
) -> None:
    value = _map()
    mutate(value)
    path = tmp_path / "candidate.source-map.json"
    path.write_text(json.dumps(value, separators=(",", ":")), encoding="utf-8")

    assert _native_map_check(path).returncode != 0


def test_native_validator_rejects_unsorted_file_and_origin_inventories(
    tmp_path: Path,
) -> None:
    values = [
        _map(generated=["z.hpp", "a.hpp"]),
        _map(
            origins=[
                {"operation": "z.op", "origin": "{}", "location": "loc(unknown)"},
                {"operation": "a.op", "origin": "{}", "location": "loc(unknown)"},
            ]
        ),
    ]
    for index, value in enumerate(values):
        path = tmp_path / f"candidate-{index}.source-map.json"
        path.write_text(json.dumps(value, separators=(",", ":")), encoding="utf-8")
        assert _native_map_check(path).returncode != 0


def test_native_validator_rejects_noncanonical_origin_and_location_attributes(
    tmp_path: Path,
) -> None:
    value = _map(
        origins=[
            {
                "operation": "ac.type_alias",
                "origin": "not an occurrence",
                "location": 'loc("root.py":0:0)',
            }
        ]
    )
    path = tmp_path / "bad-origin.source-map.json"
    path.write_text(json.dumps(value, separators=(",", ":")), encoding="utf-8")

    assert _native_map_check(path).returncode != 0


@pytest.mark.parametrize(
    "location",
    [
        "loc(unknown)",
        'loc(fused["first.py":2:3, "second.py":4:5])',
    ],
    ids=["unknown-location", "fused-location"],
)
def test_native_validator_preserves_supported_compound_location_categories(
    tmp_path: Path, location: str
) -> None:
    value = _map(
        origins=[
            {
                "operation": "ac.module",
                "origin": "{expansion = [], site = {ast_path = [], definition = @demo.root.Root}}",
                "location": location,
            }
        ]
    )
    path = tmp_path / "compound-location.source-map.json"
    path.write_text(json.dumps(value, separators=(",", ":")), encoding="utf-8")

    assert _native_map_check(path).returncode == 0


def test_group_membership_must_equal_the_map_file_list(tmp_path: Path) -> None:
    value = _map(generated=[])
    bundle = _bundle(tmp_path / "bundle", value=value)

    _assert_bundle_rejected(bundle)


def test_map_cannot_list_itself_as_a_generated_file(tmp_path: Path) -> None:
    bundle = _bundle(tmp_path / "bundle", value=_map(generated=[MAP_PATH]))

    _assert_bundle_rejected(bundle)


@pytest.mark.parametrize(
    "renamed",
    [
        "sources/demo/renamed.source-map.json",
        "wrong/sources/demo/root.source-map.json",
    ],
    ids=["wrong-basename", "extra-prefix"],
)
def test_receipt_cannot_rebind_owner_map_to_another_safe_path(
    tmp_path: Path, renamed: str
) -> None:
    bundle = _bundle(tmp_path / "bundle")
    receipt_path = bundle / "generated.json"
    receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    for row in receipt["files"]:
        if row["path"] == MAP_PATH:
            row["path"] = renamed
    group = receipt["source_groups"][0]
    group["files"] = [renamed if path == MAP_PATH else path for path in group["files"]]
    moved_map = bundle / renamed
    moved_map.parent.mkdir(parents=True, exist_ok=True)
    (bundle / MAP_PATH).rename(moved_map)
    receipt_path.write_text(
        json.dumps(receipt, separators=(",", ":")), encoding="utf-8"
    )

    _assert_bundle_rejected(bundle)


@pytest.mark.parametrize(
    "receipt,disk_files",
    [
        (
            _receipt(listed_files=[{"path": MAP_PATH, "role": "source-map"}]),
            {GENERATED_PATH: b"header"},
        ),
        (_receipt(), {GENERATED_PATH: b"header", "extra.txt": b"unlisted"}),
        (_receipt(group_files=[MAP_PATH]), {GENERATED_PATH: b"header"}),
    ],
    ids=[
        "receipt-missing-generated-member",
        "extra-disk-file",
        "group-omits-generated-member",
    ],
)
def test_receipt_directory_and_source_group_must_agree(
    tmp_path: Path, receipt: dict[str, Any], disk_files: dict[str, bytes]
) -> None:
    bundle = _bundle(tmp_path / "bundle", receipt=receipt, disk_files=disk_files)

    _assert_bundle_rejected(bundle)


def test_map_only_source_group_has_exact_empty_non_map_membership(
    tmp_path: Path,
) -> None:
    value = _map(generated=[])
    receipt = _receipt(
        group_files=[MAP_PATH], listed_files=[{"path": MAP_PATH, "role": "source-map"}]
    )
    bundle = _bundle(tmp_path / "map-only", value=value, receipt=receipt, disk_files={})
    _validate_emitted_bundle(bundle, OWNER)


@pytest.mark.parametrize("bad", [True, [], {}], ids=["boolean", "array", "dictionary"])
def test_present_wrong_typed_nominal_origin_is_rejected(tmp_path: Path, bad) -> None:
    value = _map(
        origins=[{"operation": "ac.enum", "origin": bad, "location": "loc(unknown)"}]
    )
    path = tmp_path / "wrong-type.source-map.json"
    path.write_text(json.dumps(value), encoding="utf-8")
    assert _native_map_check(path).returncode != 0

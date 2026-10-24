"""Independent v1 transport oracles; payloads intentionally have no ACIR meaning."""

from __future__ import annotations

import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path

import pytest
from pycircuit import _anchor_files as anchor
from pycircuit._publication import _PublicationError
from pycircuit._publication_fs import _PublicationFileSystemError


def _json(value):
    return json.dumps(
        value, ensure_ascii=False, sort_keys=True, separators=(",", ":")
    ).encode()


def _hash(data):
    return hashlib.sha256(data).hexdigest()


def _save(root, manifest):
    manifest["configuration_identity"] = _hash(_json(manifest["configuration"]))
    manifest["content_identity"] = _hash(
        _json({k: v for k, v in manifest.items() if k != "content_identity"})
    )
    (root / "anchor.json").write_bytes(_json(manifest) + b"\n")


def _artifact(root: Path):
    root.mkdir()
    source = {"package": "demo", "path": "counter.py"}
    symbol = '@"demo.counter.Counter"'
    spec, body, interface, graph = (
        b"spec transport\n",
        b"body transport\n",
        b"interface transport\n",
        b"graph transport\n",
    )
    variant = _hash(
        _json(
            {
                "source": source,
                "definition": symbol,
                "spec_sha256": _hash(spec),
                "body_sha256": _hash(body),
            }
        )
    )
    owner_id = _hash(_json(source))
    definition = {
        "definition": symbol,
        "source": source,
        "variant_id": variant,
        "spec": f"definitions/{variant}/spec.ac",
        "body": f"definitions/{variant}/body.ac",
        "spec_sha256": _hash(spec),
        "body_sha256": _hash(body),
        "dependencies": [],
    }
    payloads = [
        ("graph.ac", "graph", graph),
        (definition["spec"], "spec", spec),
        (definition["body"], "body", body),
        (f"sources/{owner_id}.interface.ac", "source-interface", interface),
    ]
    for name, _, data in payloads:
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    configuration = {
        "root": {"source": source, "definition": symbol, "variant_id": variant},
        "integer_arguments": [],
        "type_arguments": [],
        "domain": "default",
        "reset_semantics": "strict",
        "graph_sha256": _hash(graph),
        "definitions": [
            {
                key: definition[key]
                for key in ("definition", "variant_id", "spec_sha256", "body_sha256")
            }
        ],
    }
    manifest = {
        "kind": "pycircuit-anchor",
        "schema": 1,
        "routed_contract": 2,
        "owner": {"kind": "anchor", "source": source, "definition": symbol},
        "root": symbol,
        "configuration": configuration,
        "files": [
            {"path": name, "role": role, "sha256": _hash(data)}
            for name, role, data in sorted(payloads)
        ],
        "definitions": [definition],
        "sources": [
            {
                "source": source,
                "owner_id": owner_id,
                "interface": f"sources/{owner_id}.interface.ac",
                "body_sha256": _hash(b"original source body"),
                "interface_sha256": _hash(interface),
            }
        ],
        "producer": {
            "tool": "independent transport test",
            "version": "1",
            "pipeline": "Unicode: λ汉字",
        },
    }
    _save(root, manifest)
    return manifest


def test_portable_snapshot_is_byte_exact_and_creates_no_input_controls(tmp_path):
    source, snapshot = tmp_path / "portable", tmp_path / "snapshot"
    manifest = _artifact(source)
    before = sorted(tmp_path.iterdir())
    assert anchor._capture_anchor(source) == manifest
    assert sorted(tmp_path.iterdir()) == before
    snapshot.mkdir()
    assert anchor._capture_anchor(source, snapshot) == manifest
    expected = {
        str(p.relative_to(source)): p.read_bytes()
        for p in source.rglob("*")
        if p.is_file()
    }
    actual = {
        str(p.relative_to(snapshot)): p.read_bytes()
        for p in snapshot.rglob("*")
        if p.is_file()
    }
    assert actual == expected
    assert anchor._discover_anchor_owner(source) == (manifest["owner"], False)


@pytest.mark.parametrize("ordering", ["canonical", "reversed", "duplicate"])
def test_source_provenance_uses_canonical_json_order(tmp_path, ordering):
    source = tmp_path / "anchor"
    manifest = _artifact(source)
    additional = []
    # Native source-owner ordering compares canonical JSON bytes. The quote
    # sorts before '-' in a raw path, but its JSON escape sorts after '-'.
    for filename in ("a-.py", 'a".py'):
        owner = {"package": "demo", "path": filename}
        identity = _hash(_json(owner))
        interface = f"sources/{identity}.interface.ac"
        payload = f"empty interface transport for {filename}\n".encode()
        (source / interface).write_bytes(payload)
        manifest["files"].append(
            {"path": interface, "role": "source-interface", "sha256": _hash(payload)}
        )
        additional.append(
            {
                "source": owner,
                "owner_id": identity,
                "interface": interface,
                "body_sha256": _hash(b"empty source body"),
                "interface_sha256": _hash(payload),
            }
        )
    assert _json(additional[0]["source"]) < _json(additional[1]["source"])
    assert additional[0]["source"]["path"] > additional[1]["source"]["path"]
    if ordering == "reversed":
        additional.reverse()
    elif ordering == "duplicate":
        additional.insert(0, dict(additional[0]))
    manifest["sources"] = additional + manifest["sources"]
    manifest["files"].sort(key=lambda row: row["path"])
    _save(source, manifest)
    if ordering == "canonical":
        assert anchor._capture_anchor(source) == manifest
    else:
        diagnostic = "must be sorted" if ordering == "reversed" else "identity collides"
        with pytest.raises(_PublicationError, match=diagnostic):
            anchor._capture_anchor(source)


@pytest.mark.parametrize(
    "damage",
    [
        "unknown-field",
        "bool-version",
        "future-version",
        "old-version",
        "duplicate-definition",
        "duplicate-source",
        "duplicate-path",
        "self-inventory",
        "traversal",
        "uppercase-id",
        "integer-leading-zero",
        "type-argument",
        "wrong-root",
        "missing-inventory",
        "orphan-payload",
        "wrong-content-id",
    ],
)
def test_closed_manifest_rejections(tmp_path, damage):
    source = tmp_path / "anchor"
    manifest = _artifact(source)
    if damage == "unknown-field":
        manifest["ignored"] = 1
    elif damage == "bool-version":
        manifest["schema"] = True
    elif damage == "future-version":
        manifest["routed_contract"] = 3
    elif damage == "old-version":
        manifest["routed_contract"] = 1
    elif damage == "duplicate-definition":
        manifest["definitions"] *= 2
    elif damage == "duplicate-source":
        manifest["sources"] *= 2
    elif damage == "duplicate-path":
        manifest["files"] *= 2
    elif damage == "self-inventory":
        manifest["files"].append(
            {"path": "anchor.json", "role": "graph", "sha256": "0" * 64}
        )
    elif damage == "traversal":
        manifest["files"][0]["path"] = "../graph.ac"
    elif damage == "uppercase-id":
        manifest["definitions"][0]["variant_id"] = "A" * 64
    elif damage == "integer-leading-zero":
        manifest["configuration"]["integer_arguments"] = [
            {"name": "WIDTH", "value": "01"}
        ]
    elif damage == "type-argument":
        manifest["configuration"]["type_arguments"] = [
            {"name": "T", "type": "!ac.bits<8>"}
        ]
    elif damage == "wrong-root":
        manifest["root"] = '@"demo.counter.Other"'
    elif damage == "missing-inventory":
        manifest["files"].pop()
    elif damage == "orphan-payload":
        manifest["files"].append(
            {
                "path": f"sources/{'0' * 64}.interface.ac",
                "role": "source-interface",
                "sha256": "0" * 64,
            }
        )
    _save(source, manifest)
    if damage == "wrong-content-id":
        manifest["content_identity"] = "0" * 64
        (source / "anchor.json").write_bytes(_json(manifest) + b"\n")
    with pytest.raises(_PublicationError):
        anchor._capture_anchor(source)


@pytest.mark.parametrize(
    "damage",
    [
        "extra-file",
        "extra-dir",
        "missing-body",
        "changed-body",
        "symlink-body",
        "symlink-dir",
    ],
)
def test_closed_tree_rejections(tmp_path, damage):
    source = tmp_path / "anchor"
    manifest = _artifact(source)
    body = source / manifest["definitions"][0]["body"]
    if damage == "extra-file":
        (source / "surprise").write_bytes(b"extra")
    elif damage == "extra-dir":
        (source / "unused").mkdir()
    elif damage == "missing-body":
        body.unlink()
    elif damage == "changed-body":
        body.write_bytes(b"same-signature changed provider")
    elif damage == "symlink-body":
        external = tmp_path / "external"
        body.rename(external)
        body.symlink_to(external)
    elif damage == "symlink-dir":
        external = tmp_path / "external"
        (source / "sources").rename(external)
        (source / "sources").symlink_to(external, target_is_directory=True)
    with pytest.raises((_PublicationError, _PublicationFileSystemError, OSError)):
        anchor._capture_anchor(source)


@pytest.mark.parametrize(
    "encoding", ["duplicate-key", "pretty", "no-lf", "two-lfs", "nonfinite"]
)
def test_manifest_serialization_is_exact(tmp_path, encoding):
    source = tmp_path / "anchor"
    manifest = _artifact(source)
    path = source / "anchor.json"
    raw = path.read_bytes()
    if encoding == "duplicate-key":
        raw = b'{"schema":1,' + raw[1:]
    elif encoding == "pretty":
        raw = json.dumps(manifest, indent=2).encode() + b"\n"
    elif encoding == "no-lf":
        raw = raw[:-1]
    elif encoding == "two-lfs":
        raw += b"\n"
    elif encoding == "nonfinite":
        raw = raw.replace(b'"schema":1', b'"schema":NaN')
    path.write_bytes(raw)
    with pytest.raises(_PublicationError):
        anchor._capture_anchor(source)


@pytest.mark.parametrize(
    "limit",
    [
        "_MANIFEST_LIMIT",
        "_PAYLOAD_LIMIT",
        "_TOTAL_LIMIT",
        "_FILE_LIMIT",
        "_DIRECTORY_LIMIT",
        "_DEPTH_LIMIT",
        "_PATH_LIMIT",
    ],
)
def test_transport_limits_apply_before_native_parsing(tmp_path, monkeypatch, limit):
    source = tmp_path / "anchor"
    _artifact(source)
    monkeypatch.setattr(anchor, limit, 1)
    with pytest.raises(_PublicationError):
        anchor._capture_anchor(source)


@pytest.mark.parametrize("control", ["empty", "file", "bad-owner", "symlink"])
def test_present_broken_controls_never_downgrade_to_portable(tmp_path, control):
    source = tmp_path / "anchor"
    _artifact(source)
    metadata = tmp_path / ".anchor.pycircuit-publication"
    if control == "file":
        metadata.write_text("not a control directory")
    elif control == "symlink":
        elsewhere = tmp_path / "elsewhere"
        elsewhere.mkdir()
        metadata.symlink_to(elsewhere, target_is_directory=True)
    else:
        metadata.mkdir()
        if control == "bad-owner":
            (metadata / "owner.json").write_text("{}\n")
            (metadata / "lock").touch()
    with pytest.raises((_PublicationError, _PublicationFileSystemError, OSError)):
        anchor._discover_anchor_owner(source)


def test_payload_changed_during_read_rejects_snapshot(tmp_path, monkeypatch):
    source = tmp_path / "anchor"
    _artifact(source)
    original_read = anchor.os.read
    changed = False

    def change_after_read(descriptor, count):
        nonlocal changed
        data = original_read(descriptor, count)
        if data == b"graph transport\n" and not changed:
            changed = True
            before = os.fstat(descriptor)
            graph = source / "graph.ac"
            graph.write_bytes(b"graph corrupt!!\n")
            # Same-size writes can share the filesystem's timestamp tick. Make
            # this in-place mutation observable without changing its size/inode.
            os.utime(
                graph,
                ns=(before.st_atime_ns, before.st_mtime_ns + 2_000_000_000),
            )
            after = os.fstat(descriptor)
            assert after.st_ino == before.st_ino and after.st_size == before.st_size
            assert after.st_mtime_ns != before.st_mtime_ns
        return data

    monkeypatch.setattr(anchor.os, "read", change_after_read)
    with pytest.raises(_PublicationError, match="changed"):
        anchor._capture_anchor(source)
    assert changed


def test_inventory_order_is_canonical(tmp_path):
    source = tmp_path / "anchor"
    manifest = _artifact(source)
    manifest["files"].reverse()
    _save(source, manifest)
    with pytest.raises(_PublicationError, match="path-sorted"):
        anchor._capture_anchor(source)


@pytest.mark.skipif(os.name == "nt", reason="POSIX FIFO replacement race")
def test_raced_fifo_is_rejected_without_waiting_for_a_writer(tmp_path):
    source = tmp_path / "anchor"
    _artifact(source)
    # Isolate the old blocking behavior in a timed child. Swap after tree
    # enumeration but before the real regular-file opener, with no FIFO writer.
    script = """
import os, sys
from pathlib import Path
from pycircuit._anchor_files import _capture_anchor
from pycircuit._publication_fs import _PublicationFileSystem, _PublicationFileSystemError
original = _PublicationFileSystem._open_regular
changed = False
def race(self, path, **kwargs):
    global changed
    if path.name == 'body.ac' and not changed:
        path.unlink()
        os.mkfifo(path)
        changed = True
    return original(self, path, **kwargs)
_PublicationFileSystem._open_regular = race
try:
    _capture_anchor(Path(sys.argv[1]))
except _PublicationFileSystemError:
    assert changed
else:
    raise AssertionError('raced FIFO was accepted')
"""
    result = subprocess.run(
        [sys.executable, "-c", script, str(source)],
        env={
            **os.environ,
            "PYTHONPATH": str(Path(__file__).resolve().parents[2] / "python"),
        },
        capture_output=True,
        text=True,
        timeout=5,
    )
    assert result.returncode == 0, result.stdout + result.stderr

#!/usr/bin/env python3
"""Assemble, accept, and verify immutable pyCircuit release candidates."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import sys
import urllib.request
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
VERSION_MAP = ROOT / "packaging/sdk/version-map.json"
RELEASE_BASE = "https://github.com/PTO-ISA/pyCircuit/releases/download"


def _load(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"{path}: expected a JSON object")
    return value


def _write_json(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(value, indent=2, sort_keys=False, separators=(",", ": ")) + "\n",
        encoding="utf-8",
    )


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return "sha256:" + digest.hexdigest()


def _record(path: Path, tag: str) -> dict[str, Any]:
    return {
        "name": path.name,
        "sha256": _sha256(path),
        "size": path.stat().st_size,
        "url": f"{RELEASE_BASE}/{tag}/{path.name}",
    }


def _candidate_files(root: Path) -> dict[str, Path]:
    files: dict[str, Path] = {}
    for path in sorted(item for item in root.rglob("*") if item.is_file()):
        if path.name in files:
            raise ValueError(f"duplicate candidate asset name: {path.name}")
        files[path.name] = path
    return files


def _expected_assets(
    version_map: dict[str, Any], files: dict[str, Path]
) -> dict[str, str]:
    product = version_map["product_version"]
    agentic = version_map["distributions"]["agentic-circuit"]
    patterns = {
        "pycircuit-hisi-linux-x86_64": re.compile(
            rf"^pycircuit_hisi-{re.escape(product)}-py3-none-linux_x86_64\.whl$"
        ),
        "pycircuit-hisi-macos-arm64": re.compile(
            rf"^pycircuit_hisi-{re.escape(product)}-py3-none-macosx_[0-9]+_[0-9]+_arm64\.whl$"
        ),
        "pycircuit-semantic-core": re.compile(
            rf"^pycircuit_semantic_core-{re.escape(product)}-py3-none-any\.whl$"
        ),
        "agentic-circuit": re.compile(
            rf"^agentic_circuit-{re.escape(agentic)}-py3-none-any\.whl$"
        ),
    }
    selected: dict[str, str] = {}
    for identity, pattern in patterns.items():
        matches = [name for name in files if pattern.fullmatch(name)]
        if len(matches) != 1:
            raise ValueError(
                f"candidate requires exactly one {identity} wheel; observed {matches}"
            )
        selected[identity] = matches[0]
    expected = set(selected.values())
    for platform in version_map["platforms"]:
        platform_id = platform["id"]
        expected.update(
            {
                f"pycircuit-sdk-{product}-{platform_id}.tar.gz",
                f"pycircuit-sdk-{product}-{platform_id}.manifest.json",
            }
        )
    expected.update({"LICENSES.tar.gz", "RELEASE_NOTES.md"})
    unexpected = sorted(set(files) - expected)
    missing = sorted(expected - set(files))
    if missing:
        raise ValueError(f"missing candidate asset: {', '.join(missing)}")
    if unexpected:
        raise ValueError(f"unexpected candidate asset: {', '.join(unexpected)}")
    return selected


def aggregate(args: argparse.Namespace) -> int:
    version_map = _load(args.version_map)
    source_revision = args.source_revision.lower()
    if re.fullmatch(r"[0-9a-f]{40}", source_revision) is None:
        raise ValueError("--source-revision must be a full 40-character commit SHA")
    candidate_dir = args.candidate_dir.resolve()
    output_dir = args.out_dir.resolve()
    files = _candidate_files(candidate_dir)
    wheels = _expected_assets(version_map, files)
    if output_dir.exists() and any(output_dir.iterdir()):
        raise ValueError(f"--out-dir must be empty: {output_dir}")
    output_dir.mkdir(parents=True, exist_ok=True)
    for name in sorted(files):
        shutil.copyfile(files[name], output_dir / name)

    product = version_map["product_version"]
    tag = version_map["candidate_tag"]
    platforms: dict[str, Any] = {}
    for platform in version_map["platforms"]:
        platform_id = platform["id"]
        manifest_name = f"pycircuit-sdk-{product}-{platform_id}.manifest.json"
        manifest = _load(output_dir / manifest_name)
        if manifest.get("source_revision") != source_revision:
            raise ValueError(
                f"{manifest_name}: source_revision does not match candidate"
            )
        if manifest.get("platform", {}).get("id") != platform_id:
            raise ValueError(f"{manifest_name}: platform id does not match asset name")
        platforms[platform_id] = {
            "archive": _record(
                output_dir / f"pycircuit-sdk-{product}-{platform_id}.tar.gz", tag
            ),
            "manifest": _record(output_dir / manifest_name, tag),
        }

    wheel_records = {
        identity: _record(output_dir / name, tag)
        for identity, name in sorted(wheels.items())
    }
    release_index = {
        "schema": "pycircuit-sdk-release-index",
        "version": "1",
        "contract_epoch": version_map["contract_epoch"],
        "candidate_tag": tag,
        "product_version": product,
        "source_revision": source_revision,
        "distributions": version_map["distributions"],
        "abi": version_map["contracts"],
        "capabilities": [
            "cycle-aware-signal",
            "pyc-cpp",
            "pyc-verilog",
            "agentic-model-plan",
            "agentic-model-emit-cpp",
            "gfsim-runtime-v1",
        ],
        "platforms": platforms,
        "wheels": wheel_records,
        "licenses": _record(output_dir / "LICENSES.tar.gz", tag),
        "release_notes": _record(output_dir / "RELEASE_NOTES.md", tag),
        "checksums": {
            "name": "SHA256SUMS",
            "excludes_self": True,
            "covers_release_index": True,
        },
    }
    index_name = f"pycircuit-sdk-{product}-release-index.json"
    _write_json(output_dir / index_name, release_index)

    for platform_id, platform in platforms.items():
        hisi_key = f"pycircuit-hisi-{platform_id}"

        def lock_reference(record: dict[str, Any]) -> dict[str, Any]:
            return {
                "name": record["name"],
                "sha256": record["sha256"],
                "url": record["url"],
            }

        def lock_wheel(identity: str, version: str) -> dict[str, Any]:
            return {**lock_reference(wheel_records[identity]), "version": version}

        lock = {
            "schema": "pycircuit-sdk-lock",
            "version": "1",
            "contract_epoch": version_map["contract_epoch"],
            "release": tag,
            "source_revision": source_revision,
            "release_index": lock_reference(_record(output_dir / index_name, tag)),
            "platform": {"id": platform_id, **platform},
            "wheels": {
                "agentic-circuit": lock_wheel(
                    "agentic-circuit",
                    version_map["distributions"]["agentic-circuit"],
                ),
                "pycircuit-hisi": lock_wheel(hisi_key, product),
                "pycircuit-semantic-core": lock_wheel(
                    "pycircuit-semantic-core", product
                ),
            },
            "abi": version_map["contracts"],
            "capabilities": release_index["capabilities"],
        }
        _write_json(
            output_dir / f"pycircuit-sdk-{product}-{platform_id}.lock.json", lock
        )

    checksum_paths = sorted(
        path
        for path in output_dir.iterdir()
        if path.is_file() and path.name != "SHA256SUMS"
    )
    (output_dir / "SHA256SUMS").write_text(
        "".join(f"{_sha256(path)[7:]}  {path.name}\n" for path in checksum_paths),
        encoding="utf-8",
    )
    sys.stdout.write(
        f"release candidate: OK ({len(checksum_paths) + 1} retained assets)\n"
    )
    return 0


def _index_records(index: dict[str, Any]) -> list[dict[str, Any]]:
    records = [index["licenses"], index["release_notes"]]
    records.extend(index["wheels"].values())
    for platform in index["platforms"].values():
        records.extend((platform["archive"], platform["manifest"]))
    return records


def accept(args: argparse.Namespace) -> int:
    candidate_dir = args.candidate_dir.resolve()
    version_map = _load(args.version_map)
    index_path = candidate_dir / (
        f"pycircuit-sdk-{version_map['product_version']}-release-index.json"
    )
    index = _load(index_path)
    if index["source_revision"] != args.source_revision.lower():
        raise ValueError("release index source revision does not match requested SHA")
    for record in _index_records(index):
        path = candidate_dir / record["name"]
        if not path.is_file():
            raise ValueError(f"accepted asset is missing: {record['name']}")
        if path.stat().st_size != record["size"] or _sha256(path) != record["sha256"]:
            raise ValueError(f"accepted asset bytes changed: {record['name']}")
    checksums = candidate_dir / "SHA256SUMS"
    if not checksums.is_file():
        raise ValueError("accepted candidate is missing SHA256SUMS")
    checksum_records: dict[str, str] = {}
    for line in checksums.read_text(encoding="utf-8").splitlines():
        parts = line.split("  ", 1)
        if len(parts) != 2 or re.fullmatch(r"[0-9a-f]{64}", parts[0]) is None:
            raise ValueError("SHA256SUMS contains a malformed record")
        digest, name = parts
        if name in checksum_records or Path(name).name != name:
            raise ValueError("SHA256SUMS contains a duplicate or unsafe name")
        checksum_records[name] = digest
    actual_names = {
        path.name
        for path in candidate_dir.iterdir()
        if path.is_file() and path.name != checksums.name
    }
    if set(checksum_records) != actual_names:
        raise ValueError("SHA256SUMS does not cover the exact candidate file set")
    for name, expected in checksum_records.items():
        if _sha256(candidate_dir / name) != "sha256:" + expected:
            raise ValueError(f"SHA256SUMS mismatch: {name}")
    attestation = {
        "schema": "pycircuit-release-acceptance",
        "version": "1",
        "candidate_tag": index["candidate_tag"],
        "source_revision": index["source_revision"],
        "release_index": {"name": index_path.name, "sha256": _sha256(index_path)},
        "checksums": {"name": checksums.name, "sha256": _sha256(checksums)},
        "accepted": True,
    }
    _write_json(args.attestation, attestation)
    sys.stdout.write("release candidate acceptance: OK\n")
    return 0


def verify_published(args: argparse.Namespace) -> int:
    index = _load(args.release_index)
    checked = []
    records = {record["name"]: record for record in _index_records(index)}
    if args.candidate_dir is not None:
        for path in sorted(
            item for item in args.candidate_dir.iterdir() if item.is_file()
        ):
            records.setdefault(
                path.name,
                {
                    "name": path.name,
                    "sha256": _sha256(path),
                    "size": path.stat().st_size,
                    "url": f"{RELEASE_BASE}/{index['candidate_tag']}/{path.name}",
                },
            )
    for record in sorted(records.values(), key=lambda item: item["name"]):
        expected = f"{RELEASE_BASE}/{index['candidate_tag']}/{record['name']}"
        if record["url"] != expected:
            raise ValueError(f"unstable release URL for {record['name']}")
        with urllib.request.urlopen(record["url"], timeout=args.timeout) as response:
            content = response.read()
        digest = "sha256:" + hashlib.sha256(content).hexdigest()
        if len(content) != record["size"] or digest != record["sha256"]:
            raise ValueError(f"published bytes differ from candidate: {record['name']}")
        checked.append({"name": record["name"], "sha256": digest, "url": record["url"]})
    _write_json(
        args.attestation,
        {
            "schema": "pycircuit-release-publication-attestation",
            "version": "1",
            "candidate_tag": index["candidate_tag"],
            "source_revision": index["source_revision"],
            "assets": checked,
            "verified": True,
        },
    )
    sys.stdout.write(f"published release: OK ({len(checked)} stable assets)\n")
    return 0


def parser() -> argparse.ArgumentParser:
    result = argparse.ArgumentParser()
    result.set_defaults(version_map=VERSION_MAP)
    subparsers = result.add_subparsers(dest="command", required=True)
    aggregate_parser = subparsers.add_parser("aggregate")
    aggregate_parser.add_argument("--candidate-dir", type=Path, required=True)
    aggregate_parser.add_argument("--out-dir", type=Path, required=True)
    aggregate_parser.add_argument("--source-revision", required=True)
    aggregate_parser.add_argument("--version-map", type=Path, default=VERSION_MAP)
    aggregate_parser.set_defaults(handler=aggregate)
    accept_parser = subparsers.add_parser("accept")
    accept_parser.add_argument("--candidate-dir", type=Path, required=True)
    accept_parser.add_argument("--source-revision", required=True)
    accept_parser.add_argument("--attestation", type=Path, required=True)
    accept_parser.add_argument("--version-map", type=Path, default=VERSION_MAP)
    accept_parser.set_defaults(handler=accept)
    verify_parser = subparsers.add_parser("verify-published")
    verify_parser.add_argument("--release-index", type=Path, required=True)
    verify_parser.add_argument("--candidate-dir", type=Path)
    verify_parser.add_argument("--attestation", type=Path, required=True)
    verify_parser.add_argument("--timeout", type=float, default=60.0)
    verify_parser.set_defaults(handler=verify_published)
    return result


def main(argv: list[str] | None = None) -> int:
    try:
        args = parser().parse_args(argv)
        return int(args.handler(args))
    except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError) as error:
        sys.stderr.write(f"error: {error}\n")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

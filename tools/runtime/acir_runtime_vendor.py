#!/usr/bin/env python3
"""Check that accepted runtime entries are self-contained and redistributable.

The check is intentionally local and deterministic: every path is resolved
under the catalog root, every promoted entry must carry a complete dependency
closure and a license file, and an optional release manifest can pin SHA-256
digests for the vendored files.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Any, Mapping, Sequence


SCHEMA = "acir-runtime-vendoring-check-v0.1"


def _load(path: Path) -> Mapping[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, Mapping):
        raise ValueError(f"expected JSON object: {path}")
    return value


def _resolve(root: Path, value: str) -> Path:
    path = Path(value)
    if path.is_absolute():
        raise ValueError(f"vendored paths must be relative: {value}")
    resolved = (root / path).resolve()
    try:
        resolved.relative_to(root.resolve())
    except ValueError as exc:
        raise ValueError(f"vendored path escapes catalog root: {value}") from exc
    return resolved


def check_vendoring(catalog_path: Path, manifest_path: Path | None = None) -> dict[str, Any]:
    catalog = _load(catalog_path)
    entries = catalog.get("entries")
    if not isinstance(entries, list):
        raise ValueError("catalog entries must be an array")
    root = catalog_path.parent.resolve()
    manifest = _load(manifest_path) if manifest_path else {}
    expected_hashes = manifest.get("sha256", {}) if isinstance(manifest, Mapping) else {}
    if expected_hashes and not isinstance(expected_hashes, Mapping):
        raise ValueError("manifest sha256 must be an object")
    rows: list[dict[str, Any]] = []
    errors: list[str] = []
    for entry in entries:
        if not isinstance(entry, Mapping) or entry.get("status") != "accepted":
            continue
        name = str(entry.get("name", ""))
        listed = entry.get("files")
        closure = entry.get("dependency_closure")
        paths = [str(item) for item in listed] if isinstance(listed, list) else []
        license_files = []
        source_files = []
        if isinstance(closure, Mapping):
            license_files = [str(item) for item in closure.get("license_files", [])]
            source_files = [str(item) for item in closure.get("source_files", [])]
            if closure.get("status") != "complete":
                errors.append(f"{name}: dependency_closure.status is not complete")
        elif entry.get("oracle"):
            errors.append(f"{name}: semantic component has no dependency_closure")
        all_paths = list(dict.fromkeys(paths + license_files))
        missing: list[str] = []
        bad_hashes: list[str] = []
        for raw in all_paths:
            try:
                path = _resolve(root, raw)
            except ValueError as exc:
                errors.append(f"{name}: {exc}")
                continue
            if not path.is_file():
                missing.append(raw)
                continue
            expected = expected_hashes.get(raw) if isinstance(expected_hashes, Mapping) else None
            if expected:
                actual = hashlib.sha256(path.read_bytes()).hexdigest()
                if actual != str(expected):
                    bad_hashes.append(raw)
        if missing:
            errors.append(f"{name}: missing files: {', '.join(missing)}")
        if bad_hashes:
            errors.append(f"{name}: SHA-256 mismatch: {', '.join(bad_hashes)}")
        if license_files and not any(_resolve(root, item).is_file() for item in license_files):
            errors.append(f"{name}: no vendored license file exists")
        rows.append({"name": name, "status": "passed" if not missing and not bad_hashes else "failed", "files": len(all_paths), "source_files": source_files, "license_files": license_files})
    return {"schema": SCHEMA, "catalog": str(catalog_path), "manifest": str(manifest_path) if manifest_path else None, "summary": {"entries": len(rows), "passed": sum(row["status"] == "passed" for row in rows), "failed": sum(row["status"] == "failed" for row in rows), "errors": len(errors), "status": "passed" if not errors and all(row["status"] == "passed" for row in rows) else "failed"}, "results": rows, "errors": errors}


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--catalog", type=Path, default=Path("library/verilog/catalog.json"))
    parser.add_argument("--manifest", type=Path, default=Path("library/verilog/manifests/parameterized-components-v0.3.json"))
    parser.add_argument("--report", type=Path, default=Path(".pycircuit_out/runtime-vendoring-check/report.json"))
    args = parser.parse_args(argv)
    try:
        catalog = args.catalog.resolve()
        manifest = args.manifest.resolve() if args.manifest else None
        report = check_vendoring(catalog, manifest)
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"acir-runtime vendor-check: error: {exc}", file=sys.stderr)
        return 2
    print(json.dumps(report["summary"], sort_keys=True))
    return 0 if report["summary"]["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())

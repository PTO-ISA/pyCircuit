#!/usr/bin/env python3
"""Unified, bounded command line for the Agentic Circuit RTL runtime.

The subcommands intentionally compose the existing deterministic tools rather
than reimplementing their expensive logic.  A normal workflow is
``scan -> validate -> promote -> verify-catalog``; ``list`` is a cheap catalog
query suitable for shell scripts and CI diagnostics.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any, Sequence

try:  # Package import (tests/embedding) and direct tools/ execution.
    from .acir_runtime_crawler import main as crawl_main
    from .acir_runtime_full_validation import main as validation_main
    from .acir_runtime_promotion import main as promotion_main
    from .acir_runtime_verify import main as verify_runtime_main
    from .acir_runtime_functional import main as functional_main
    from .acir_runtime_adapter import main as adapter_main
    from .acir_runtime_vendor import main as vendor_main
    from .acir_runtime_stage_candidates import main as stage_candidates_main
except ImportError:  # pragma: no cover - exercised by the script entrypoint.
    from acir_runtime_crawler import main as crawl_main
    from acir_runtime_full_validation import main as validation_main
    from acir_runtime_promotion import main as promotion_main
    from acir_runtime_verify import main as verify_runtime_main
    from acir_runtime_functional import main as functional_main
    from acir_runtime_adapter import main as adapter_main
    from acir_runtime_vendor import main as vendor_main
    from acir_runtime_stage_candidates import main as stage_candidates_main


def _load_catalog(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict) or not isinstance(value.get("entries"), list):
        raise ValueError("catalog must be an object with an entries array")
    return value


def _list_catalog(args: argparse.Namespace) -> int:
    catalog_path = args.catalog.resolve()
    catalog = _load_catalog(catalog_path)
    entries = catalog["entries"]
    if args.status:
        entries = [entry for entry in entries if entry.get("status") == args.status]
    if args.family:
        entries = [entry for entry in entries if entry.get("family") == args.family]
    if args.json:
        print(json.dumps(entries, indent=2, sort_keys=True))
    else:
        for entry in entries:
            print(f"{entry.get('name', entry.get('module', '<unnamed>'))}\t{entry.get('status', '')}\t{entry.get('provider', '')}\t{entry.get('module', '')}")
    return 0


def _verify_catalog(args: argparse.Namespace) -> int:
    catalog_path = args.catalog.resolve()
    try:
        catalog = _load_catalog(catalog_path)
        names: set[str] = set()
        missing: list[str] = []
        lock_name = catalog.get("catalog_lock")
        lock: dict[str, Any] = {}
        if lock_name:
            lock_path = catalog_path.parent / str(lock_name)
            lock = json.loads(lock_path.read_text(encoding="utf-8"))
            if not isinstance(lock, dict) or not isinstance(lock.get("sources"), dict):
                raise ValueError("catalog lock must contain a sources object")
        for entry in catalog["entries"]:
            if not isinstance(entry, dict):
                raise ValueError("catalog entries must be objects")
            name = str(entry.get("name", ""))
            if not name or name in names:
                raise ValueError(f"catalog entry name is missing or duplicated: {name!r}")
            names.add(name)
            if entry.get("status") == "accepted":
                module = str(entry.get("module", ""))
                if not module:
                    raise ValueError(f"accepted entry {name!r} has no module")
                # Legacy entries use a root-level module file.  New entries may
                # list a relative wrapper path explicitly.
                wrapper = entry.get("wrapper")
                if wrapper:
                    path = Path(str(wrapper))
                    if not path.is_absolute():
                        path = catalog_path.parent / path
                else:
                    path = catalog_path.parent / "verilog" / f"{module}.v"
                if not path.is_file():
                    missing.append(str(path))
                files = entry.get("files", [])
                interface = entry.get("interface")
                if not isinstance(interface, dict) or not interface.get("wrapper_module"):
                    raise ValueError(f"accepted entry {name!r} has no canonical wrapper_module")
                ports = interface.get("ports")
                if not isinstance(ports, list) or not ports:
                    raise ValueError(f"accepted entry {name!r} has no canonical ports")
                for port in ports:
                    if not isinstance(port, dict) or not all(port.get(key) for key in ("name", "direction", "width")):
                        raise ValueError(f"accepted entry {name!r} has an invalid canonical port")
                if not isinstance(files, list) or not files:
                    raise ValueError(f"accepted entry {name!r} has no dependency file list")
                if wrapper and wrapper not in files:
                    raise ValueError(f"accepted entry {name!r} wrapper is not in dependency file list")
                for listed in files:
                    listed_path = Path(str(listed))
                    if not listed_path.is_absolute():
                        listed_path = catalog_path.parent / listed_path
                    if not listed_path.is_file():
                        missing.append(str(listed_path))
                validation = entry.get("validation")
                if isinstance(validation, dict) and validation.get("manifest"):
                    manifest_path = catalog_path.parent / str(validation["manifest"])
                    if not manifest_path.is_file():
                        missing.append(str(manifest_path))
                provenance = entry.get("provenance")
                source_name = str(entry.get("source", ""))
                if lock and isinstance(provenance, dict):
                    locked = lock["sources"].get(source_name)
                    if isinstance(locked, dict) and (provenance.get("commit") != locked.get("commit") or provenance.get("license") != locked.get("license")):
                        raise ValueError(f"catalog provenance does not match lock for {name!r}")
        if missing:
            raise ValueError("missing accepted runtime files: " + ", ".join(missing))
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"acir-runtime verify-catalog: error: {exc}", file=sys.stderr)
        return 2
    print(json.dumps({"catalog": str(catalog_path), "entries": len(catalog["entries"]), "status": "valid"}, sort_keys=True))
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)
    scan = sub.add_parser("scan", help="discover and structurally validate bounded RTL candidates")
    scan.add_argument("args", nargs=argparse.REMAINDER, help="arguments passed to acir-runtime-crawl")
    validate = sub.add_parser("validate", help="run the serial full validation campaign")
    validate.add_argument("args", nargs=argparse.REMAINDER, help="arguments passed to acir-runtime-full-validation")
    promote = sub.add_parser("promote", help="derive a reviewable runtime candidate manifest")
    promote.add_argument("args", nargs=argparse.REMAINDER, help="arguments passed to acir-runtime-promote")
    verify_runtime = sub.add_parser("verify-runtime", help="run Verilator/Yosys gates for packaged accepted entries")
    verify_runtime.add_argument("args", nargs=argparse.REMAINDER, help="arguments passed to acir-runtime-verify")
    functional = sub.add_parser("functional", help="run packaged runtime functional oracles")
    functional.add_argument("args", nargs=argparse.REMAINDER, help="arguments passed to acir-runtime-functional")
    adapt = sub.add_parser("adapt", help="package structurally validated candidates behind stable adapters")
    adapt.add_argument("args", nargs=argparse.REMAINDER, help="arguments passed to acir-runtime-adapt")
    vendor = sub.add_parser("vendor-check", help="verify vendored source closures, licenses and release hashes")
    vendor.add_argument("args", nargs=argparse.REMAINDER, help="arguments passed to acir-runtime-vendor")
    stage = sub.add_parser("stage-candidates", help="vendor structurally validated candidates into a reviewable bundle")
    stage.add_argument("args", nargs=argparse.REMAINDER, help="arguments passed to acir-runtime-stage-candidates")
    list_parser = sub.add_parser("list", help="list entries in a runtime catalog")
    list_parser.add_argument("--catalog", type=Path, default=Path("library/verilog/catalog.json"))
    list_parser.add_argument("--status")
    list_parser.add_argument("--family")
    list_parser.add_argument("--json", action="store_true")
    verify = sub.add_parser("verify-catalog", help="check accepted entries and packaged files")
    verify.add_argument("--catalog", type=Path, default=Path("library/verilog/catalog.json"))
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    raw = list(sys.argv[1:] if argv is None else argv)
    # Forward tool-specific options before parsing the small catalog commands;
    # otherwise argparse treats e.g. ``--report`` as an option of this
    # dispatcher rather than of the promotion tool.  This also keeps each
    # delegated command's complete --help surface intact.
    if raw and raw[0] == "scan":
        return crawl_main(raw[1:])
    if raw and raw[0] == "validate":
        return validation_main(raw[1:])
    if raw and raw[0] == "promote":
        return promotion_main(raw[1:])
    if raw and raw[0] == "verify-runtime":
        return verify_runtime_main(raw[1:])
    if raw and raw[0] == "functional":
        return functional_main(raw[1:])
    if raw and raw[0] == "adapt":
        return adapter_main(raw[1:])
    if raw and raw[0] == "vendor-check":
        return vendor_main(raw[1:])
    if raw and raw[0] == "stage-candidates":
        return stage_candidates_main(raw[1:])
    args = build_parser().parse_args(raw)
    if args.command == "list":
        try:
            return _list_catalog(args)
        except (OSError, ValueError, json.JSONDecodeError) as exc:
            print(f"acir-runtime list: error: {exc}", file=sys.stderr)
            return 2
    if args.command == "verify-catalog":
        return _verify_catalog(args)
    raise AssertionError(args.command)


if __name__ == "__main__":
    raise SystemExit(main())

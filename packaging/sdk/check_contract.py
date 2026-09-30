#!/usr/bin/env python3
"""Validate current SDK documents using the frozen C3 schemas."""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCHEMAS = {
    "pycircuit-sdk-version-map": "sdk-version-map.schema.json",
    "pycircuit-sdk-platform-manifest": "sdk-manifest.schema.json",
    "pycircuit-sdk-release-index": "release-index.schema.json",
    "pycircuit-sdk-lock": "consumer-lock.schema.json",
}
DEFAULTS = [
    ROOT / "packaging/sdk/version-map.json",
    *sorted((ROOT / "packaging/sdk/examples").glob("*.json")),
]


def reject_content_identity(schema: dict, document: dict) -> None:
    active = json.dumps({"schema": schema, "document": document}).lower()
    for token in ("finger" + "print", "sha" + "256", "check" + "sum", "di" + "gest"):
        if token in active:
            raise ValueError(f"forbidden content-identity token: {token}")


def validate(path: Path) -> None:
    document = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(document, dict) or document.get("schema") not in SCHEMAS:
        raise ValueError(f"unknown SDK document kind: {path}")
    schema = json.loads(
        (ROOT / "schemas/agentic-circuit" / SCHEMAS[document["schema"]]).read_text(
            encoding="utf-8"
        )
    )
    reject_content_identity(schema, document)
    from jsonschema import Draft202012Validator

    Draft202012Validator.check_schema(schema)
    Draft202012Validator(schema).validate(document)
    rows = document.get("files", [])
    paths = [row["path"] for row in rows]
    if len(paths) != len(set(paths)):
        raise ValueError(f"duplicate SDK inventory path: {path}")
    forbidden = {
        "pycc",
        "pyc-opt",
        "acc",
        "acc.py",
        "agentic-circuit",
        "acir-opt",
        "libgfsim.a",
        "AgenticCircuitConfig.cmake",
    }
    for relative in paths:
        if Path(relative).name in forbidden:
            raise ValueError(f"retired SDK inventory asset: {relative}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--document", type=Path, action="append")
    args = parser.parse_args()
    for path in args.document or DEFAULTS:
        validate(path)
    sys.stdout.write("SDK contract: OK\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

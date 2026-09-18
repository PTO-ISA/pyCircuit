#!/usr/bin/env python3
"""Validate the ACC-only SDK inventory and release contracts."""

from __future__ import annotations

import copy
import json
import sys
from pathlib import Path

try:
    from jsonschema import Draft202012Validator
except ModuleNotFoundError:  # Repository contract lane can run without dev extras.
    Draft202012Validator = None

ROOT = Path(__file__).resolve().parents[2]
SCHEMA_DIR = ROOT / "schemas/agentic-circuit"
EXAMPLE_DIR = ROOT / "packaging/sdk/examples"
CONTRACTS = {
    "pycircuit-sdk-version-map": (
        "sdk-version-map.schema.json",
        ROOT / "packaging/sdk/version-map.json",
    ),
    "pycircuit-sdk-platform-manifest": (
        "sdk-manifest.schema.json",
        EXAMPLE_DIR / "sdk-platform-manifest.example.json",
    ),
    "pycircuit-sdk-release-index": (
        "release-index.schema.json",
        EXAMPLE_DIR / "release-index.example.json",
    ),
    "pycircuit-sdk-lock": (
        "consumer-lock.schema.json",
        EXAMPLE_DIR / "consumer-lock.example.json",
    ),
    "agentic-circuit-emitted-cost": (
        "emitted-cost.schema.json",
        EXAMPLE_DIR / "emitted-cost.example.json",
    ),
}


def load(path: Path) -> dict:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"{path}: expected an object")
    return value


def errors_for(schema: dict, document: dict) -> list[str]:
    if Draft202012Validator is None:
        return []
    return [
        error.message
        for error in sorted(
            Draft202012Validator(schema).iter_errors(document),
            key=lambda item: list(item.absolute_path),
        )
    ]


def main() -> int:
    loaded: dict[str, tuple[dict, dict]] = {}
    failures: list[str] = []
    for identity, (schema_name, document_path) in CONTRACTS.items():
        schema = load(SCHEMA_DIR / schema_name)
        document = load(document_path)
        if Draft202012Validator is not None:
            Draft202012Validator.check_schema(schema)
        if document.get("schema") != identity:
            failures.append(f"{document_path}: expected schema {identity}")
            continue
        failures.extend(
            f"{document_path}: {message}" for message in errors_for(schema, document)
        )
        loaded[identity] = (schema, document)

    forbidden_tokens = (
        "finger" + "print",
        "sha" + "256",
        "check" + "sum",
        "di" + "gest",
    )
    for identity, (schema, document) in loaded.items():
        active = json.dumps({"schema": schema, "document": document}).lower()
        for token in forbidden_tokens:
            if token in active:
                failures.append(f"{identity}: forbidden content-identity token {token}")

    sdk_schema, sdk = loaded["pycircuit-sdk-platform-manifest"]
    duplicate = copy.deepcopy(sdk)
    duplicate["files"].append(copy.deepcopy(duplicate["files"][0]))
    paths = [record["path"] for record in duplicate["files"]]
    if len(paths) == len(set(paths)):
        failures.append("SDK example did not exercise duplicate inventory path")
    if sdk_schema.get("additionalProperties") is not False:
        failures.append("SDK schema accepts unknown top-level fields")

    if failures:
        for failure in failures:
            print(f"ERROR: {failure}", file=sys.stderr)
        return 1
    print(f"SDK contract: OK ({len(CONTRACTS)} schemas, {len(CONTRACTS)} documents)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

"""Packaged schema discovery and exact capability assembly."""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Mapping

from ._canonical_json import JsonValue, canonical_json_bytes, sha256_bytes
from ._native_api import NativeCapabilities, capabilities as native_capabilities
from ._package_data import resource_directory


EXACT_CONTRACT_IDENTITIES: dict[str, str] = {
    "acpy": "acpy@0.1",
    "acir": "acir@0.1",
    "acsim": "acsim@0.1",
    "cli": "agentic-circuit-cli@0.1",
    "component_schema": "agentic-circuit-component@0.1",
    "opcode_catalog": "agentic-circuit-opcode-catalog@0.4",
    "block_spec": "agentic-circuit-block-spec@0.4",
    "cxx_source_contract": "gfsim-cxx20@0.1",
    "pto_trace": "pto-trace@0.1",
    "diagnostic": "agentic-circuit-diagnostic@0.1",
    "build_manifest": "agentic-circuit-build-manifest@0.1",
    "run_manifest": "agentic-circuit-run-manifest@0.1",
    "run_result": "agentic-circuit-run-result@0.1",
}


def schema_root() -> Path:
    return resource_directory("schemas") / "agentic-circuit"


def diagnostics_catalog_path() -> Path:
    return schema_root() / "diagnostics" / "diagnostics.json"


def load_json(path: Path) -> dict[str, JsonValue]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if type(value) is not dict:
        raise ValueError(f"packaged resource is not an object: {path.name}")
    return value


def standard_library_catalog() -> dict[str, JsonValue]:
    return load_json(schema_root() / "stdlib" / "catalog.json")


def opcode_catalog() -> dict[str, JsonValue]:
    return load_json(schema_root() / "opcodes.json")


def block_spec() -> dict[str, JsonValue]:
    return load_json(schema_root() / "blocks.json")


def diagnostic_catalog() -> dict[str, JsonValue]:
    return load_json(diagnostics_catalog_path())


@dataclass(frozen=True, slots=True)
class CapabilityDocument:
    contract_identities: Mapping[str, str]
    items: tuple[Mapping[str, JsonValue], ...]
    compiler_build_id: str
    runtime_build_id: str

    def to_json(self) -> dict[str, JsonValue]:
        return {
            "schema": "agentic-circuit-capabilities",
            "version": "0.1",
            "contract_epoch": "0.4",
            "contract_identities": dict(self.contract_identities),
            "items": [dict(item) for item in self.items],
            "compiler_build_id": self.compiler_build_id,
            "runtime_build_id": self.runtime_build_id,
        }


def _synthetic_fingerprint(kind: str, name: str) -> str:
    return sha256_bytes(f"{kind}:{name}@0.1".encode("utf-8"))


def _base_items(
    catalog: dict[str, JsonValue],
    opcodes: dict[str, JsonValue],
    blocks: dict[str, JsonValue],
) -> list[dict[str, JsonValue]]:
    entries = catalog.get("entries")
    if type(entries) is not list:
        raise ValueError("standard-library catalog entries are invalid")
    items: list[dict[str, JsonValue]] = []
    for raw in entries:
        if type(raw) is not dict:
            raise ValueError("standard-library catalog entry is invalid")
        name = raw.get("canonical_name")
        availability = raw.get("availability")
        fingerprint = raw.get("schema_fingerprint")
        if not all(type(value) is str for value in (name, availability, fingerprint)):
            raise ValueError("standard-library catalog entry fields are invalid")
        schema_path = str(raw["schema_path"]).removeprefix("schemas/")
        schema = load_json(schema_root().parent / schema_path)
        kind = "protocol" if schema.get("family") == "protocol" else "component"
        items.append(
            {
                "kind": kind,
                "name": name,
                "availability": availability,
                "schema_fingerprint": fingerprint,
                "implementation_fingerprint": None,
            }
        )
    catalog_fingerprint = sha256_bytes(canonical_json_bytes(catalog))
    items.append(
        {
            "kind": "provider",
            "name": "ac",
            "availability": "available",
            "schema_fingerprint": catalog_fingerprint,
            "implementation_fingerprint": None,
        }
    )
    opcode_entries = opcodes.get("entries")
    if type(opcode_entries) is not list:
        raise ValueError("official opcode catalog entries are invalid")
    for entry in opcode_entries:
        if type(entry) is not dict or type(entry.get("operation")) is not str:
            raise ValueError("official opcode catalog entry is invalid")
        items.append(
            {
                "kind": "opcode",
                "name": entry["operation"],
                "availability": "available",
                "schema_fingerprint": sha256_bytes(canonical_json_bytes(entry)),
                "implementation_fingerprint": None,
            }
        )
    block_entries = blocks.get("blocks")
    if type(block_entries) is not list:
        raise ValueError("high-level BlockSpec entries are invalid")
    for entry in block_entries:
        if type(entry) is not dict or type(entry.get("operation")) is not str:
            raise ValueError("high-level BlockSpec entry is invalid")
        items.append(
            {
                "kind": "block",
                "name": entry["operation"],
                "availability": "available",
                "schema_fingerprint": sha256_bytes(canonical_json_bytes(entry)),
                "implementation_fingerprint": None,
            }
        )
    for profile in ("custom", "fast", "validated"):
        items.append(
            {
                "kind": "policy",
                "name": profile,
                "availability": "available",
                "schema_fingerprint": _synthetic_fingerprint("policy", profile),
                "implementation_fingerprint": None,
            }
        )
    items.append(
        {
            "kind": "interface",
            "name": "ac.Stream",
            "availability": "available",
            "schema_fingerprint": _synthetic_fingerprint("interface", "ac.Stream"),
            "implementation_fingerprint": None,
        }
    )
    for output_format in ("dot", "json", "jsonl", "text"):
        items.append(
            {
                "kind": "output_format",
                "name": output_format,
                "availability": "available",
                "schema_fingerprint": _synthetic_fingerprint(
                    "output_format", output_format
                ),
                "implementation_fingerprint": None,
            }
        )
    return items


def capability_document(
    native: NativeCapabilities | None = None,
) -> CapabilityDocument:
    catalog = standard_library_catalog()
    opcodes = opcode_catalog()
    blocks = block_spec()
    native = native or native_capabilities()
    items = _base_items(catalog, opcodes, blocks)
    native_items = {(item.get("kind"), item.get("name")): item for item in native.items}
    for item in items:
        override = native_items.get((item["kind"], item["name"]))
        if override is not None:
            if override.get("availability") in (
                "available",
                "declared_unavailable",
            ):
                item["availability"] = override["availability"]
            if override.get("implementation_fingerprint") is not None:
                item["implementation_fingerprint"] = override[
                    "implementation_fingerprint"
                ]
        if (
            item["availability"] == "available"
            and item["implementation_fingerprint"] is None
        ):
            build_id = (
                native.compiler_build_id
                if item["kind"] in ("policy", "output_format")
                else native.runtime_build_id
            )
            identity = {
                "kind": item["kind"],
                "name": item["name"],
                "schema_fingerprint": item["schema_fingerprint"],
                "build_id": build_id,
            }
            item["implementation_fingerprint"] = sha256_bytes(
                canonical_json_bytes(identity)
            )
    items.sort(key=lambda item: (str(item["kind"]), str(item["name"])))
    return CapabilityDocument(
        contract_identities=EXACT_CONTRACT_IDENTITIES,
        items=tuple(items),
        compiler_build_id=native.compiler_build_id,
        runtime_build_id=native.runtime_build_id,
    )

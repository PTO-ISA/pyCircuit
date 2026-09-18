"""Packaged schema discovery and exact capability assembly."""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Mapping

from ._canonical_json import JsonValue
from ._native_api import NativeCapabilities
from ._native_api import capabilities as native_capabilities
from ._package_data import resource_directory

EXACT_CONTRACT_IDENTITIES: dict[str, str] = {
    "acpy": "acpy@0.1",
    "acir": "acir@0.1",
    "cli": "agentic-circuit-cli@0.1",
    "component_schema": "agentic-circuit-component@0.1",
    "opcode_catalog": "agentic-circuit-opcode-catalog@0.5",
    "block_spec": "agentic-circuit-block-spec@0.5",
    "cxx_source_contract": "gfsim-cxx20@0.1",
    "diagnostic": "agentic-circuit-diagnostic@0.1",
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

    def to_json(self) -> dict[str, JsonValue]:
        return {
            "schema": "agentic-circuit-capabilities",
            "version": "0.1",
            "contract_identities": dict(self.contract_identities),
            "items": [dict(item) for item in self.items],
        }


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
        if not all(type(value) is str for value in (name, availability)):
            raise ValueError("standard-library catalog entry fields are invalid")
        schema_path = str(raw["schema_path"]).removeprefix("schemas/")
        schema = load_json(schema_root().parent / schema_path)
        kind = "protocol" if schema.get("family") == "protocol" else "component"
        items.append(
            {
                "kind": kind,
                "name": name,
                "availability": availability,
            }
        )
    items.append(
        {
            "kind": "provider",
            "name": "ac",
            "availability": "available",
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
            }
        )
    for profile in ("custom", "fast", "validated"):
        items.append(
            {
                "kind": "policy",
                "name": profile,
                "availability": "available",
            }
        )
    items.append(
        {
            "kind": "interface",
            "name": "ac.Stream",
            "availability": "available",
        }
    )
    for output_format in ("dot", "json", "jsonl", "text"):
        items.append(
            {
                "kind": "output_format",
                "name": output_format,
                "availability": "available",
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
    items.sort(key=lambda item: (str(item["kind"]), str(item["name"])))
    return CapabilityDocument(
        contract_identities=EXACT_CONTRACT_IDENTITIES,
        items=tuple(items),
    )

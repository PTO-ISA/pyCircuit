"""Read-only packaged schema and capability queries."""

from __future__ import annotations

from typing import NoReturn

from .._capabilities import (
    block_spec,
    capability_document,
    diagnostic_catalog,
    load_json,
    opcode_catalog,
    schema_root,
    standard_library_catalog,
)
from .._canonical_json import JsonValue
from .._diagnostics import Diagnostic
from .._output import OutputSink
from .._workspace import UserInputError


def _fail(message: str) -> NoReturn:
    raise UserInputError(
        Diagnostic(
            stage="schema",
            code="ACPY-SCHEMA-001",
            severity="error",
            message=message,
        )
    )


def _catalog_entries() -> list[dict[str, JsonValue]]:
    entries = standard_library_catalog().get("entries")
    if type(entries) is not list or not all(type(item) is dict for item in entries):
        raise ValueError("packaged standard-library catalog is invalid")
    return entries


def _component_records(kind: str) -> list[tuple[str, dict[str, JsonValue]]]:
    records: list[tuple[str, dict[str, JsonValue]]] = []
    for entry in _catalog_entries():
        path = entry.get("schema_path")
        name = entry.get("canonical_name")
        if type(path) is not str or type(name) is not str:
            raise ValueError("packaged catalog entry is invalid")
        record = load_json(schema_root().parents[1] / path)
        is_protocol = record.get("family") == "protocol"
        if (kind == "protocol") == is_protocol:
            records.append((name, record))
    records.sort(key=lambda item: item[0])
    return records


def _select(
    records: list[tuple[str, dict[str, JsonValue]]], name: str
) -> dict[str, JsonValue]:
    candidates = [record for identity, record in records if name == identity]
    if len(candidates) != 1:
        _fail(f"schema identity is unknown: {name}")
    return candidates[0]


def _listing(kind: str, names: list[str]) -> dict[str, JsonValue]:
    return {
        "schema": "agentic-circuit-schema-list",
        "version": "0.1",
        "contract_epoch": "0.4",
        "kind": kind,
        "items": sorted(names),
    }


def _diagnostics(name: str | None) -> dict[str, JsonValue]:
    entries = diagnostic_catalog().get("entries")
    if type(entries) is not list or not all(type(item) is dict for item in entries):
        raise ValueError("packaged diagnostic catalog is invalid")
    if name is None:
        return _listing("diagnostic", [str(item["code"]) for item in entries])
    matches = [item for item in entries if item.get("code") == name]
    if len(matches) != 1:
        _fail(f"diagnostic code is unknown: {name}")
    return matches[0]


def _opcodes(name: str | None) -> dict[str, JsonValue]:
    entries = opcode_catalog().get("entries")
    if type(entries) is not list or not all(type(item) is dict for item in entries):
        raise ValueError("packaged official opcode catalog is invalid")
    if name is None:
        return _listing("opcode", [str(item["operation"]) for item in entries])
    matches = [item for item in entries if item.get("operation") == name]
    if len(matches) != 1:
        _fail(f"opcode identity is unknown: {name}")
    return matches[0]


def _blocks(name: str | None) -> dict[str, JsonValue]:
    entries = block_spec().get("blocks")
    if type(entries) is not list or not all(type(item) is dict for item in entries):
        raise ValueError("packaged high-level BlockSpec is invalid")
    if name is None:
        return _listing("block", [str(item["operation"]) for item in entries])
    matches = [item for item in entries if item.get("operation") == name]
    if len(matches) != 1:
        _fail(f"block identity is unknown: {name}")
    return matches[0]


def run(arguments: object, sink: OutputSink) -> int:
    kind = getattr(arguments, "kind")
    name = getattr(arguments, "name", None)
    if kind == "capabilities":
        if name is not None:
            _fail("capabilities does not accept a name")
        document = capability_document().to_json()
    elif kind in ("component", "protocol"):
        records = _component_records(kind)
        document = (
            _listing(kind, [identity for identity, _ in records])
            if name is None
            else _select(records, name)
        )
    elif kind == "diagnostic":
        document = _diagnostics(name)
    elif kind == "opcode":
        document = _opcodes(name)
    elif kind == "block":
        document = _blocks(name)
    elif kind == "interface":
        names = ["ac.Stream"]
        if name is None:
            document = _listing(kind, names)
        elif name == "ac.Stream":
            document = {
                "schema": "agentic-circuit-interface-definition",
                "version": "0.1",
                "contract_epoch": "0.4",
                "canonical_name": "ac.Stream",
                "availability": "available",
            }
        else:
            _fail(f"interface identity is unknown: {name}")
    elif kind == "packet":
        if name is not None:
            _fail(f"packet identity is unknown: {name}")
        document = _listing(kind, [])
    else:
        _fail(f"schema kind is unknown: {kind}")
    sink.result(document, human=str(document))
    return 0

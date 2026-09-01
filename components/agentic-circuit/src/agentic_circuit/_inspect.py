"""Deterministic read-only views over verified frontend and build artifacts."""

from __future__ import annotations

import json
from dataclasses import dataclass
from types import MappingProxyType
from typing import Literal, Mapping, TypeAlias

from ._canonical_json import JsonValue, canonical_json_bytes, sha256_bytes


InspectionKind: TypeAlias = Literal[
    "graph",
    "hierarchy",
    "ports",
    "resources",
    "address-map",
    "protocols",
    "specialization",
    "artifacts",
]
InspectionFormat: TypeAlias = Literal["json", "dot", "text"]


@dataclass(frozen=True, slots=True)
class InspectionRequest:
    kind: InspectionKind
    system: str
    path: str | None = None
    format: InspectionFormat = "json"


@dataclass(frozen=True, slots=True)
class InspectionResult:
    kind: InspectionKind
    system: str
    path: str | None
    records: tuple[Mapping[str, JsonValue], ...]

    def to_json(self) -> dict[str, JsonValue]:
        return {
            "schema": "agentic-circuit-inspection",
            "version": "0.1",
            "contract_epoch": "0.4",
            "kind": self.kind,
            "system": self.system,
            "path": self.path,
            "records": [dict(record) for record in self.records],
        }


class InspectionError(ValueError):
    pass


def _document(data: bytes, label: str) -> dict[str, object]:
    try:
        value = json.loads(data)
    except (UnicodeError, json.JSONDecodeError) as error:
        raise InspectionError(f"{label} is invalid JSON: {error}") from error
    if type(value) is not dict:
        raise InspectionError(f"{label} must be a JSON object")
    return value


def _properties(entity: dict[str, object]) -> dict[str, JsonValue]:
    values = entity.get("properties")
    if type(values) is not list:
        raise InspectionError("ACPy entity properties are invalid")
    result: dict[str, JsonValue] = {}
    for item in values:
        if type(item) is not dict or set(item) != {"name", "value"}:
            raise InspectionError("ACPy entity property is invalid")
        name = item["name"]
        if type(name) is not str or name in result:
            raise InspectionError("ACPy entity property name is invalid")
        result[name] = item["value"]
    return result


def _entity_name(entity: dict[str, object]) -> str | None:
    properties = _properties(entity)
    for key in ("name", "instance_name"):
        value = properties.get(key)
        if type(value) is str and value:
            return value
    return None


def _entities(acpy: dict[str, object]) -> tuple[dict[str, object], ...]:
    if (
        acpy.get("schema") != "agentic-circuit-acpy"
        or acpy.get("version") != "0.1"
        or acpy.get("contract_epoch") != "0.4"
        or type(acpy.get("entities")) is not list
    ):
        raise InspectionError("ACPy has an invalid envelope")
    entities = tuple(acpy["entities"])
    if not all(type(entity) is dict for entity in entities):
        raise InspectionError("ACPy entities are invalid")
    expected = tuple(f"e{index}" for index in range(len(entities)))
    if tuple(entity.get("id") for entity in entities) != expected:
        raise InspectionError("ACPy entity IDs are not canonical")
    return entities


def _hierarchy_paths(
    entities: tuple[dict[str, object], ...],
) -> tuple[dict[str, str], dict[str, str]]:
    object_kinds = {"module", "scope", "call", "collection", "process"}
    paths: dict[str, str] = {}
    owners: dict[str, str] = {}
    for entity in entities:
        identifier = entity["id"]
        assert isinstance(identifier, str)
        parent = entity.get("parent")
        inherited = owners.get(parent) if isinstance(parent, str) else None
        kind = entity.get("kind")
        if kind not in object_kinds:
            if inherited is not None:
                owners[identifier] = inherited
            continue
        name = _entity_name(entity)
        if name is None:
            raise InspectionError(f"hierarchy entity {identifier} has no stable name")
        path = f"{inherited}.{name}" if inherited else name
        if path in paths.values():
            raise InspectionError(f"duplicate hierarchy path {path!r}")
        paths[identifier] = path
        owners[identifier] = path
    return paths, owners


def _record(**values: JsonValue) -> Mapping[str, JsonValue]:
    return MappingProxyType(dict(values))


def _selected(path: str, requested: str | None) -> bool:
    return requested is None or path == requested or path.startswith(requested + ".")


def _hierarchy_records(
    entities: tuple[dict[str, object], ...],
    paths: dict[str, str],
    requested: str | None,
) -> tuple[Mapping[str, JsonValue], ...]:
    records = []
    for entity in entities:
        identifier = entity["id"]
        if identifier not in paths or not _selected(paths[identifier], requested):
            continue
        schema = entity.get("schema_ref")
        records.append(
            _record(
                id=identifier,
                kind=str(entity["kind"]),
                path=paths[identifier],
                schema=(schema.get("identity") if type(schema) is dict else None),
            )
        )
    return tuple(sorted(records, key=lambda item: str(item["path"])))


def _graph_records(
    entities: tuple[dict[str, object], ...],
    paths: dict[str, str],
    owners: dict[str, str],
    requested: str | None,
) -> tuple[Mapping[str, JsonValue], ...]:
    nodes = []
    for entity in entities:
        identifier = str(entity["id"])
        path = paths.get(identifier)
        if path is None or not _selected(path, requested):
            continue
        nodes.append(
            _record(record="node", id=identifier, path=path, kind=str(entity["kind"]))
        )
    edges: set[tuple[str, str, str, str | None]] = set()
    for entity in entities:
        target = owners.get(str(entity["id"]))
        if target is None or not _selected(target, requested):
            continue
        parent = entity.get("parent")
        source = owners.get(parent) if isinstance(parent, str) else None
        if source is not None and source != target and _selected(source, requested):
            edges.add((source, target, "ownership", None))
        uses = entity.get("uses")
        if type(uses) is list:
            for used in uses:
                source = owners.get(used) if isinstance(used, str) else None
                if source is not None and source != target and _selected(source, requested):
                    edges.add((source, target, "data", None))
    edge_records = tuple(
        _record(record="edge", source=source, target=target, kind=kind, port=port)
        for source, target, kind, port in sorted(edges)
    )
    return tuple(sorted(nodes, key=lambda item: str(item["path"]))) + edge_records


def _port_records(
    entities: tuple[dict[str, object], ...],
    owners: dict[str, str],
    requested: str | None,
) -> tuple[Mapping[str, JsonValue], ...]:
    directions = {"arg": "input", "result": "output", "bind": "binding"}
    records = []
    for entity in entities:
        direction = directions.get(str(entity.get("kind")))
        owner = owners.get(str(entity["id"]))
        if direction is None or owner is None or not _selected(owner, requested):
            continue
        records.append(
            _record(
                id=str(entity["id"]),
                path=owner,
                name=_entity_name(entity) or str(entity["id"]),
                direction=direction,
                type=entity.get("type"),
            )
        )
    return tuple(sorted(records, key=lambda item: (str(item["path"]), str(item["name"]))))


def _matching_records(
    entities: tuple[dict[str, object], ...],
    owners: dict[str, str],
    requested: str | None,
    tokens: tuple[str, ...],
) -> tuple[Mapping[str, JsonValue], ...]:
    records = []
    for entity in entities:
        owner = owners.get(str(entity["id"]))
        if owner is None or not _selected(owner, requested):
            continue
        schema = entity.get("schema_ref")
        schema_identity = schema.get("identity") if type(schema) is dict else None
        searchable = " ".join(
            (
                str(entity.get("kind", "")),
                str(entity.get("type", "")),
                str(schema_identity or ""),
                " ".join(_properties(entity)),
            )
        ).lower()
        if not any(token in searchable for token in tokens):
            continue
        records.append(
            _record(
                id=str(entity["id"]),
                path=owner,
                kind=str(entity["kind"]),
                schema=schema_identity,
                type=entity.get("type"),
            )
        )
    return tuple(sorted(records, key=lambda item: (str(item["path"]), str(item["id"]))))


def _specialization_records(
    entities: tuple[dict[str, object], ...],
    paths: dict[str, str],
    requested: str | None,
) -> tuple[Mapping[str, JsonValue], ...]:
    records = []
    for entity in entities:
        identifier = str(entity["id"])
        if identifier not in paths or not _selected(paths[identifier], requested):
            continue
        schema = entity.get("schema_ref")
        records.append(
            _record(
                id=identifier,
                path=paths[identifier],
                kind=str(entity["kind"]),
                schema=(schema.get("identity") if type(schema) is dict else None),
                schema_fingerprint=(
                    schema.get("fingerprint") if type(schema) is dict else None
                ),
                properties=_properties(entity),
            )
        )
    return tuple(sorted(records, key=lambda item: str(item["path"])))


def _artifact_records(
    acpy: dict[str, object], acir: bytes, build_manifest: dict[str, object] | None
) -> tuple[Mapping[str, JsonValue], ...]:
    if build_manifest is not None:
        artifacts = build_manifest.get("artifacts")
        if type(artifacts) is not list:
            raise InspectionError("build manifest artifacts are invalid")
        records = [
            _record(
                path="build-manifest.json",
                kind="build_manifest",
                build_fingerprint=str(build_manifest.get("build_fingerprint")),
            )
        ]
        for artifact in artifacts:
            if type(artifact) is not dict or set(artifact) != {"path", "kind", "sha256"}:
                raise InspectionError("build manifest artifact is invalid")
            records.append(
                _record(
                    path=str(artifact["path"]),
                    kind=str(artifact["kind"]),
                    sha256=str(artifact["sha256"]),
                )
            )
        return tuple(sorted(records, key=lambda item: str(item["path"])))
    sources = acpy.get("sources")
    if type(sources) is not list:
        raise InspectionError("ACPy sources are invalid")
    records = [
        _record(path="model.ac.mlir", kind="acir", sha256=sha256_bytes(acir))
    ]
    for source in sources:
        if type(source) is not dict or set(source) != {"path", "sha256"}:
            raise InspectionError("ACPy source artifact is invalid")
        records.append(
            _record(
                path=str(source["path"]),
                kind="source",
                sha256=str(source["sha256"]),
            )
        )
    return tuple(sorted(records, key=lambda item: str(item["path"])))


def inspect_model(
    acpy_bytes: bytes,
    acir_bytes: bytes,
    request: InspectionRequest,
    *,
    build_manifest: dict[str, object] | None = None,
) -> InspectionResult:
    acpy = _document(acpy_bytes, "ACPy")
    entities = _entities(acpy)
    paths, owners = _hierarchy_paths(entities)
    if request.path is not None and request.path not in set(paths.values()):
        raise InspectionError(f"unknown canonical hierarchy path {request.path!r}")
    if request.kind == "graph":
        records = _graph_records(entities, paths, owners, request.path)
    elif request.kind == "hierarchy":
        records = _hierarchy_records(entities, paths, request.path)
    elif request.kind == "ports":
        records = _port_records(entities, owners, request.path)
    elif request.kind == "resources":
        records = _matching_records(
            entities, owners, request.path, ("resource", "queue", "storage")
        )
    elif request.kind == "address-map":
        records = _matching_records(
            entities, owners, request.path, ("address", "range")
        )
    elif request.kind == "protocols":
        records = _matching_records(
            entities, owners, request.path, ("protocol", "flow", "endpoint")
        )
    elif request.kind == "specialization":
        records = _specialization_records(entities, paths, request.path)
    else:
        records = _artifact_records(acpy, acir_bytes, build_manifest)
    return InspectionResult(request.kind, request.system, request.path, records)


def inspect_build(
    build_manifest: dict[str, object], request: InspectionRequest
) -> InspectionResult:
    if request.kind != "artifacts" or request.path is not None:
        raise InspectionError("build inspection requires the unscoped artifacts view")
    return InspectionResult(
        request.kind,
        request.system,
        None,
        _artifact_records({}, b"", build_manifest),
    )


def render_json(result: InspectionResult) -> bytes:
    return canonical_json_bytes(result.to_json()) + b"\n"


def render_dot(result: InspectionResult) -> str:
    if result.kind != "graph":
        raise InspectionError("Graphviz DOT is available only for graph inspection")
    lines = ["digraph agentic_circuit {"]
    for record in result.records:
        if record.get("record") != "node":
            continue
        identifier = json.dumps(str(record["path"]), ensure_ascii=False)
        label = json.dumps(f"{record['path']}\n{record['kind']}", ensure_ascii=False)
        lines.append(f"  {identifier} [label={label}];")
    for record in result.records:
        if record.get("record") != "edge":
            continue
        source = json.dumps(str(record["source"]), ensure_ascii=False)
        target = json.dumps(str(record["target"]), ensure_ascii=False)
        label = json.dumps(str(record["kind"]), ensure_ascii=False)
        lines.append(f"  {source} -> {target} [label={label}];")
    lines.append("}")
    return "\n".join(lines) + "\n"


def render_text(result: InspectionResult) -> str:
    if result.kind == "hierarchy":
        return "".join(f"{record['path']}\n" for record in result.records)
    return "".join(
        canonical_json_bytes(dict(record)).decode("utf-8") + "\n"
        for record in result.records
    )

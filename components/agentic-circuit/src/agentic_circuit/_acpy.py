"""Closed, immutable ACPy semantic document records."""

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Literal, TypeAlias

from ._canonical_json import JsonValue, canonical_json_bytes, utf16_sort_key
from ._diagnostics import Diagnostic, SourceSpan
from ._static_eval import FrozenMap, StaticValue


EntityKind: TypeAlias = Literal[
    "system",
    "module",
    "scope",
    "arg",
    "call",
    "result",
    "get_result",
    "bind",
    "static_if",
    "static_for",
    "collection",
    "get_static",
    "return",
    "capture",
    "escape",
    "process",
]
_ENTITY_KINDS = {
    "system",
    "module",
    "scope",
    "arg",
    "call",
    "result",
    "get_result",
    "bind",
    "static_if",
    "static_for",
    "collection",
    "get_static",
    "return",
    "capture",
    "escape",
    "process",
}
_DIGEST = re.compile(r"^sha256:[0-9a-f]{64}$")
_PROPERTY_NAME = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")


def _span_json(source: SourceSpan) -> dict[str, JsonValue]:
    return {
        "file": source.file,
        "start_line": source.start_line,
        "start_column": source.start_column,
        "end_line": source.end_line,
        "end_column": source.end_column,
    }


def _static_json(value: StaticValue) -> JsonValue:
    if value is None or type(value) in (bool, int, float, str):
        return value
    if isinstance(value, tuple):
        return [_static_json(item) for item in value]
    if isinstance(value, FrozenMap):
        return {key: _static_json(item) for key, item in value.entries}
    raise ValueError(f"unsupported ACPy property value {type(value).__name__}")


@dataclass(frozen=True, slots=True)
class SourceFile:
    path: str
    sha256: str

    def to_json(self) -> dict[str, JsonValue]:
        return {"path": self.path, "sha256": self.sha256}


@dataclass(frozen=True, slots=True)
class SchemaRef:
    identity: str
    fingerprint: str

    def to_json(self) -> dict[str, JsonValue]:
        return {"identity": self.identity, "fingerprint": self.fingerprint}


@dataclass(frozen=True, slots=True)
class Property:
    name: str
    value: StaticValue

    def to_json(self) -> dict[str, JsonValue]:
        return {"name": self.name, "value": _static_json(self.value)}


@dataclass(frozen=True, slots=True)
class Entity:
    id: str
    kind: EntityKind
    source: SourceSpan | None
    parent: str | None
    scope: str
    type: str | None
    definition: str | None
    uses: tuple[str, ...]
    schema_ref: SchemaRef | None
    properties: tuple[Property, ...]

    def to_json(self) -> dict[str, JsonValue]:
        return {
            "id": self.id,
            "kind": self.kind,
            "source": _span_json(self.source) if self.source is not None else None,
            "parent": self.parent,
            "scope": self.scope,
            "type": self.type,
            "definition": self.definition,
            "uses": list(self.uses),
            "schema_ref": (
                self.schema_ref.to_json() if self.schema_ref is not None else None
            ),
            "properties": [property_.to_json() for property_ in self.properties],
        }


@dataclass(frozen=True, slots=True)
class AcpyDocument:
    entry: str
    sources: tuple[SourceFile, ...]
    entities: tuple[Entity, ...]
    schema: str = "agentic-circuit-acpy"
    version: str = "0.1"
    contract_epoch: str = "0.4"

    def to_json(self) -> dict[str, JsonValue]:
        return {
            "schema": self.schema,
            "version": self.version,
            "contract_epoch": self.contract_epoch,
            "entry": self.entry,
            "sources": [source.to_json() for source in self.sources],
            "entities": [entity.to_json() for entity in self.entities],
        }

    def _diagnostic(self, message: str, source: SourceSpan | None = None) -> Diagnostic:
        return Diagnostic(
            stage="acpy-verify",
            code="ACPY-TYPE-DOCUMENT",
            severity="error",
            message=message,
            source=source,
        )

    def verify(self) -> tuple[Diagnostic, ...]:
        errors: list[Diagnostic] = []
        if (self.schema, self.version, self.contract_epoch) != (
            "agentic-circuit-acpy",
            "0.1",
            "0.4",
        ):
            errors.append(self._diagnostic("ACPy schema identity must be epoch 0.4"))

        expected_ids = [f"e{index}" for index in range(len(self.entities))]
        actual_ids = [entity.id for entity in self.entities]
        if actual_ids != expected_ids:
            errors.append(self._diagnostic("ACPy entity IDs must be dense and ordered"))
        entity_ids = set(actual_ids)
        if self.entry not in entity_ids:
            errors.append(self._diagnostic("ACPy entry does not resolve"))

        source_paths = [source.path for source in self.sources]
        if source_paths != sorted(source_paths, key=utf16_sort_key) or len(
            source_paths
        ) != len(set(source_paths)):
            errors.append(self._diagnostic("ACPy sources must be unique and ordered"))
        for source in self.sources:
            if not source.path or not _DIGEST.fullmatch(source.sha256):
                errors.append(self._diagnostic("ACPy source record is invalid"))

        for entity in self.entities:
            if entity.kind not in _ENTITY_KINDS:
                errors.append(self._diagnostic("ACPy entity kind is invalid", entity.source))
            if not entity.scope:
                errors.append(self._diagnostic("ACPy entity scope is empty", entity.source))
            if entity.source is not None and entity.source.file not in source_paths:
                errors.append(
                    self._diagnostic("ACPy entity source does not resolve", entity.source)
                )
            references = [entity.parent, entity.definition, *entity.uses]
            if any(reference is not None and reference not in entity_ids for reference in references):
                errors.append(
                    self._diagnostic("ACPy entity reference does not resolve", entity.source)
                )
            if len(entity.uses) != len(set(entity.uses)):
                errors.append(self._diagnostic("ACPy uses must be unique", entity.source))
            property_names = [property_.name for property_ in entity.properties]
            if property_names != sorted(property_names, key=utf16_sort_key) or len(
                property_names
            ) != len(set(property_names)):
                errors.append(
                    self._diagnostic("ACPy properties must be unique and ordered", entity.source)
                )
            if any(not _PROPERTY_NAME.fullmatch(name) for name in property_names):
                errors.append(self._diagnostic("ACPy property name is invalid", entity.source))
            if entity.schema_ref is not None and (
                not entity.schema_ref.identity
                or not _DIGEST.fullmatch(entity.schema_ref.fingerprint)
            ):
                errors.append(self._diagnostic("ACPy schema reference is invalid", entity.source))

        try:
            canonical_json_bytes(self.to_json())
        except ValueError as error:
            errors.append(self._diagnostic(f"ACPy contains non-I-JSON data: {error}"))
        return tuple(errors)

    def canonical_bytes(self) -> bytes:
        errors = self.verify()
        if errors:
            raise ValueError("; ".join(error.message for error in errors))
        return canonical_json_bytes(self.to_json())


class EntityAllocator:
    __slots__ = ("_entities",)

    def __init__(self) -> None:
        self._entities: list[Entity] = []

    def allocate(
        self,
        *,
        kind: EntityKind,
        scope: str,
        source: SourceSpan | None = None,
        parent: str | None = None,
        type: str | None = None,
        definition: str | None = None,
        uses: tuple[str, ...] = (),
        schema_ref: SchemaRef | None = None,
        properties: tuple[Property, ...] = (),
    ) -> Entity:
        entity = Entity(
            id=f"e{len(self._entities)}",
            kind=kind,
            source=source,
            parent=parent,
            scope=scope,
            type=type,
            definition=definition,
            uses=uses,
            schema_ref=schema_ref,
            properties=properties,
        )
        self._entities.append(entity)
        return entity

    def freeze(self) -> tuple[Entity, ...]:
        return tuple(self._entities)

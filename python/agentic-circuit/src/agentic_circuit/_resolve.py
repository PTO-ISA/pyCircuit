"""Exact component port and result inference for normalized calls."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Literal

from ._diagnostics import SourceSpan
from ._schemas import ComponentSchema
from ._static_eval import StaticValue


ValueCategory = Literal["static", "flow", "endpoint", "resource", "result"]


@dataclass(frozen=True, slots=True)
class ValueVersion:
    source_name: str
    version: int
    category: ValueCategory
    type_key: str
    producer: str | None
    ownership: Literal["owned", "borrowed", "shared"] = "borrowed"

    @property
    def name(self) -> str:
        return f"{self.source_name}#{self.version}"


@dataclass(frozen=True, slots=True)
class PortBinding:
    port: str
    value: ValueVersion
    binding_kind: Literal["flow", "endpoint", "resource_ref"]
    role: str


@dataclass(frozen=True, slots=True)
class ResultBinding:
    result: str
    value: ValueVersion


@dataclass(frozen=True, slots=True)
class UnresolvedCall:
    entity_key: str
    instance_name: str
    static_arguments: tuple[tuple[str, StaticValue], ...]
    port_values: tuple[tuple[str, ValueVersion], ...]
    result_values: tuple[ValueVersion, ...]
    source: SourceSpan


@dataclass(frozen=True, slots=True)
class ResolvedCall:
    entity_key: str
    schema: ComponentSchema
    instance_name: str
    static_arguments: tuple[tuple[str, StaticValue], ...]
    inputs: tuple[PortBinding, ...]
    results: tuple[ResultBinding, ...]
    source: SourceSpan


class ResolutionError(ValueError):
    """Raised when exact port or result inference fails."""


def resolve_call(call: UnresolvedCall, schema: ComponentSchema) -> ResolvedCall:
    port_values = dict(call.port_values)
    if set(port_values) != {port.name for port in schema.ports}:
        raise ResolutionError("ACPY-CALL-003: exact port binding failed")
    expected_categories = {
        "flow": "flow",
        "endpoint": "endpoint",
        "resource_ref": "resource",
    }
    inputs: list[PortBinding] = []
    for port in schema.ports:
        value = port_values[port.name]
        expected = expected_categories[port.binding_kind]
        if value.category != expected:
            raise ResolutionError(
                f"ACPY-CALL-004: port {port.name!r} expects {expected}"
            )
        inputs.append(PortBinding(port.name, value, port.binding_kind, port.role))
    if len(call.result_values) != len(schema.results):
        raise ResolutionError("ACPY-CALL-005: result shape does not match schema")
    results = tuple(
        ResultBinding(result.name, value)
        for result, value in zip(schema.results, call.result_values, strict=True)
    )
    return ResolvedCall(
        entity_key=call.entity_key,
        schema=schema,
        instance_name=call.instance_name,
        static_arguments=call.static_arguments,
        inputs=tuple(inputs),
        results=results,
        source=call.source,
    )

"""Closed component schemas and their generated Python call signatures."""

from __future__ import annotations

import inspect
import json
import re
from dataclasses import dataclass
from pathlib import Path
from types import MappingProxyType
from typing import Literal

from ._canonical_json import JsonValue, canonical_json_bytes, sha256_bytes
from ._static_eval import validate_ijson_value
from ._types import SymbolicValue


Availability = Literal["available", "declared_unavailable"]
_TOP_LEVEL_KEYS = {
    "schema_kind",
    "schema_version",
    "contract_epoch",
    "canonical_name",
    "family",
    "provider_namespace",
    "stability",
    "cpp_binding",
    "static_parameters",
    "bindings",
    "results",
    "resources",
    "address_behavior",
    "protocol_contracts",
    "effect",
    "activation",
    "observation",
    "schema_fingerprint",
}
_NESTED_KEYS = {
    "cpp_binding": {
        "header",
        "symbol",
        "language",
        "concept",
        "toolchain_target",
        "functional_policy",
    },
    "static_parameters": {
        "name",
        "acir_type",
        "required",
        "default",
        "constraint",
        "cpp_mapping",
    },
    "bindings": {
        "name",
        "binding_kind",
        "acir_type",
        "direction",
        "role",
        "cardinality",
        "linearity",
        "ownership",
        "delegation",
        "result_mapping",
    },
    "results": {"name", "acir_type", "source_binding", "linearity", "ownership"},
    "resources": {
        "name",
        "class",
        "capacity",
        "lanes",
        "issue_width",
        "initiation_interval",
        "latency_policy",
        "reservation_owner",
        "transaction_classes",
        "statistics",
    },
    "address_behavior": {
        "consumes",
        "produces",
        "ranges",
        "translation",
        "routing",
        "unmapped",
    },
    "protocol_contracts": {
        "protocol",
        "role",
        "ordering",
        "delivery",
        "correlation",
        "completion",
        "failure",
    },
    "effect": {
        "kind",
        "requirements",
        "guarantees",
        "observable_effects",
        "failure_behavior",
    },
    "activation": {"sources", "predicate", "wakeup_contract", "quiescence"},
    "observation": {"probes", "counters", "gauges", "histograms"},
}
_NAME = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
_QUALIFIED_NAME = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*(\.[A-Za-z_][A-Za-z0-9_]*)+$")
_NAMESPACE_NAME = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*(\.[A-Za-z_][A-Za-z0-9_]*)*$")


class SchemaError(ValueError):
    """Raised when a catalog or component schema violates the frozen contract."""


def _object_pairs(pairs: list[tuple[str, JsonValue]]) -> dict[str, JsonValue]:
    result: dict[str, JsonValue] = {}
    for key, value in pairs:
        if key in result:
            raise SchemaError(f"duplicate JSON field {key!r}")
        result[key] = value
    return result


def _load_json(path: Path) -> dict[str, JsonValue]:
    try:
        value = json.loads(
            path.read_text(encoding="utf-8"),
            object_pairs_hook=_object_pairs,
            parse_constant=lambda token: (_ for _ in ()).throw(
                SchemaError(f"non-finite JSON number {token}")
            ),
        )
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise SchemaError(f"cannot load schema {path}: {error}") from error
    if type(value) is not dict:
        raise SchemaError(f"schema {path} must contain an object")
    return value


def _exact_keys(
    value: object, expected: set[str], context: str
) -> dict[str, JsonValue]:
    if type(value) is not dict:
        raise SchemaError(f"{context} must be an object")
    record = value
    if set(record) != expected:
        raise SchemaError(f"{context} has unknown or missing fields")
    return record


def _record_list(
    value: object, keys: set[str], context: str
) -> list[dict[str, JsonValue]]:
    if type(value) is not list:
        raise SchemaError(f"{context} must be an array")
    return [
        _exact_keys(item, keys, f"{context}[{index}]")
        for index, item in enumerate(value)
    ]


def _string(value: object, context: str, *, qualified: bool = False) -> str:
    pattern = _QUALIFIED_NAME if qualified else _NAME
    if type(value) is not str or not pattern.fullmatch(value):
        raise SchemaError(f"{context} is invalid")
    return value


def _nonempty_string(value: object, context: str) -> str:
    if type(value) is not str or not value:
        raise SchemaError(f"{context} must be a non-empty string")
    return value


def _enum(value: object, allowed: set[str], context: str) -> str:
    if type(value) is not str or value not in allowed:
        raise SchemaError(f"{context} is invalid")
    return value


def _string_list(value: object, context: str) -> list[str]:
    if type(value) is not list or any(type(item) is not str for item in value):
        raise SchemaError(f"{context} must be an array of strings")
    if len(value) != len(set(value)):
        raise SchemaError(f"{context} must contain unique strings")
    return value


def _positive_integer(value: object, context: str, *, allow_zero: bool = False) -> int:
    minimum = 0 if allow_zero else 1
    if type(value) is not int or value < minimum:
        raise SchemaError(f"{context} must be an integer >= {minimum}")
    return value


def _validate_component_fields(record: dict[str, JsonValue], name: str) -> None:
    _string(record["canonical_name"], f"{name}.canonical_name", qualified=True)
    _string(record["family"], f"{name}.family")
    provider_namespace = record["provider_namespace"]
    if type(provider_namespace) is not str or not _NAMESPACE_NAME.fullmatch(
        provider_namespace
    ):
        raise SchemaError(f"{name}.provider_namespace is invalid")
    _enum(
        record["stability"],
        {"experimental", "provisional", "stable"},
        f"{name}.stability",
    )

    cpp_binding = record["cpp_binding"]
    if cpp_binding is not None:
        binding = _exact_keys(
            cpp_binding, _NESTED_KEYS["cpp_binding"], f"{name}.cpp_binding"
        )
        for field in ("header", "symbol", "concept", "toolchain_target"):
            _nonempty_string(binding[field], f"{name}.cpp_binding.{field}")
        if binding["language"] != "c++20":
            raise SchemaError(f"{name}.cpp_binding.language is invalid")
        _enum(
            binding["functional_policy"],
            {"required", "optional", "none"},
            f"{name}.cpp_binding.functional_policy",
        )

    for index, parameter in enumerate(
        _record_list(
            record["static_parameters"],
            _NESTED_KEYS["static_parameters"],
            f"{name}.static_parameters",
        )
    ):
        context = f"{name}.static_parameters[{index}]"
        _string(parameter["name"], f"{context}.name")
        _nonempty_string(parameter["acir_type"], f"{context}.acir_type")
        if type(parameter["required"]) is not bool:
            raise SchemaError(f"{context}.required must be boolean")
        if parameter["constraint"] is not None:
            _nonempty_string(parameter["constraint"], f"{context}.constraint")
        _enum(
            parameter["cpp_mapping"],
            {"template_argument", "constexpr_argument", "constructor_constant"},
            f"{context}.cpp_mapping",
        )

    for index, binding in enumerate(
        _record_list(record["bindings"], _NESTED_KEYS["bindings"], f"{name}.bindings")
    ):
        context = f"{name}.bindings[{index}]"
        _string(binding["name"], f"{context}.name")
        _enum(
            binding["binding_kind"],
            {"flow", "endpoint", "resource_ref"},
            f"{context}.binding_kind",
        )
        _nonempty_string(binding["acir_type"], f"{context}.acir_type")
        _enum(binding["direction"], {"in", "out", "inout"}, f"{context}.direction")
        _nonempty_string(binding["role"], f"{context}.role")
        cardinality = binding["cardinality"]
        if type(cardinality) is int:
            _positive_integer(cardinality, f"{context}.cardinality", allow_zero=True)
        else:
            _nonempty_string(cardinality, f"{context}.cardinality")
        _enum(
            binding["linearity"],
            {"linear", "affine", "unrestricted"},
            f"{context}.linearity",
        )
        _enum(
            binding["ownership"],
            {"owned", "borrowed", "shared"},
            f"{context}.ownership",
        )
        _enum(
            binding["delegation"],
            {"forbidden", "allowed", "required"},
            f"{context}.delegation",
        )
        if binding["result_mapping"] is not None:
            _string(binding["result_mapping"], f"{context}.result_mapping")

    for index, result in enumerate(
        _record_list(record["results"], _NESTED_KEYS["results"], f"{name}.results")
    ):
        context = f"{name}.results[{index}]"
        _string(result["name"], f"{context}.name")
        _nonempty_string(result["acir_type"], f"{context}.acir_type")
        if result["source_binding"] is not None:
            _string(result["source_binding"], f"{context}.source_binding")
        _enum(
            result["linearity"],
            {"linear", "affine", "unrestricted"},
            f"{context}.linearity",
        )
        _enum(
            result["ownership"], {"owned", "borrowed", "shared"}, f"{context}.ownership"
        )

    for index, resource in enumerate(
        _record_list(
            record["resources"], _NESTED_KEYS["resources"], f"{name}.resources"
        )
    ):
        context = f"{name}.resources[{index}]"
        _string(resource["name"], f"{context}.name")
        for field in ("class", "latency_policy", "reservation_owner"):
            _nonempty_string(resource[field], f"{context}.{field}")
        capacity = resource["capacity"]
        if type(capacity) is int:
            _positive_integer(capacity, f"{context}.capacity", allow_zero=True)
        else:
            _nonempty_string(capacity, f"{context}.capacity")
        for field in ("lanes", "issue_width", "initiation_interval"):
            _positive_integer(resource[field], f"{context}.{field}")
        _string_list(resource["transaction_classes"], f"{context}.transaction_classes")
        _string_list(resource["statistics"], f"{context}.statistics")

    address = record["address_behavior"]
    if address is not None:
        address = _exact_keys(
            address, _NESTED_KEYS["address_behavior"], f"{name}.address_behavior"
        )
        for field in ("consumes", "produces", "ranges"):
            _string_list(address[field], f"{name}.address_behavior.{field}")
        for field in ("translation", "routing", "unmapped"):
            _nonempty_string(address[field], f"{name}.address_behavior.{field}")

    for index, protocol in enumerate(
        _record_list(
            record["protocol_contracts"],
            _NESTED_KEYS["protocol_contracts"],
            f"{name}.protocol_contracts",
        )
    ):
        for field in _NESTED_KEYS["protocol_contracts"]:
            _nonempty_string(
                protocol[field], f"{name}.protocol_contracts[{index}].{field}"
            )

    effect = _exact_keys(record["effect"], _NESTED_KEYS["effect"], f"{name}.effect")
    effect_kind = _enum(effect["kind"], {"pure", "stateful"}, f"{name}.effect.kind")
    for field in ("requirements", "guarantees", "observable_effects"):
        _string_list(effect[field], f"{name}.effect.{field}")
    _nonempty_string(effect["failure_behavior"], f"{name}.effect.failure_behavior")

    activation = record["activation"]
    if effect_kind == "pure" and activation is not None:
        raise SchemaError(f"{name}.activation must be null for a pure component")
    if effect_kind != "pure" and activation is None:
        raise SchemaError(f"{name}.activation is required for a stateful component")
    if activation is not None:
        activation = _exact_keys(
            activation, _NESTED_KEYS["activation"], f"{name}.activation"
        )
        _string_list(activation["sources"], f"{name}.activation.sources")
        _nonempty_string(activation["predicate"], f"{name}.activation.predicate")
        _string_list(
            activation["wakeup_contract"], f"{name}.activation.wakeup_contract"
        )
        _nonempty_string(activation["quiescence"], f"{name}.activation.quiescence")

    observation = _exact_keys(
        record["observation"], _NESTED_KEYS["observation"], f"{name}.observation"
    )
    for field in ("probes", "counters", "gauges", "histograms"):
        _string_list(observation[field], f"{name}.observation.{field}")


@dataclass(frozen=True, slots=True)
class PortSchema:
    name: str
    binding_kind: Literal["flow", "endpoint", "resource_ref"]
    acir_type: str
    direction: Literal["in", "out", "inout"]
    role: str
    cardinality: int | str


@dataclass(frozen=True, slots=True)
class ResultSchema:
    name: str
    acir_type: str
    source_binding: str | None
    ownership: Literal["owned", "borrowed", "shared"] = "owned"


@dataclass(frozen=True, slots=True)
class ParameterSchema:
    name: str
    acir_type: str
    required: bool
    default: JsonValue
    constraint: str | None


@dataclass(frozen=True, slots=True)
class ComponentSchema:
    identity: str
    fingerprint: str
    ports: tuple[PortSchema, ...]
    results: tuple[ResultSchema, ...]
    parameters: tuple[ParameterSchema, ...]
    availability: Availability
    effect_kind: Literal["pure", "stateful"] = "stateful"
    external_binding: str | None = None


@dataclass(frozen=True, slots=True)
class PendingCall:
    schema: ComponentSchema
    arguments: tuple[tuple[str, object], ...]


def _component_schema(
    record: dict[str, JsonValue],
    availability: Availability,
    expected_name: str,
    *,
    external: bool = False,
) -> ComponentSchema:
    _exact_keys(record, _TOP_LEVEL_KEYS, expected_name)
    if (
        record["schema_kind"] != "agentic-circuit-component"
        or record["schema_version"] != "0.1"
        or record["contract_epoch"] != "0.4"
        or record["canonical_name"] != expected_name
    ):
        raise SchemaError(f"component identity mismatch for {expected_name}")

    _validate_component_fields(record, expected_name)

    cpp_binding = record["cpp_binding"]
    if cpp_binding is not None:
        _exact_keys(
            cpp_binding, _NESTED_KEYS["cpp_binding"], f"{expected_name}.cpp_binding"
        )
    parameter_records = _record_list(
        record["static_parameters"],
        _NESTED_KEYS["static_parameters"],
        f"{expected_name}.static_parameters",
    )
    binding_records = _record_list(
        record["bindings"], _NESTED_KEYS["bindings"], f"{expected_name}.bindings"
    )
    result_records = _record_list(
        record["results"], _NESTED_KEYS["results"], f"{expected_name}.results"
    )
    _record_list(
        record["resources"], _NESTED_KEYS["resources"], f"{expected_name}.resources"
    )
    address_behavior = record["address_behavior"]
    if address_behavior is not None:
        _exact_keys(
            address_behavior,
            _NESTED_KEYS["address_behavior"],
            f"{expected_name}.address_behavior",
        )
    _record_list(
        record["protocol_contracts"],
        _NESTED_KEYS["protocol_contracts"],
        f"{expected_name}.protocol_contracts",
    )
    _exact_keys(record["effect"], _NESTED_KEYS["effect"], f"{expected_name}.effect")
    activation = record["activation"]
    if activation is not None:
        _exact_keys(
            activation, _NESTED_KEYS["activation"], f"{expected_name}.activation"
        )
    _exact_keys(
        record["observation"],
        _NESTED_KEYS["observation"],
        f"{expected_name}.observation",
    )

    digest_record = dict(record)
    fingerprint = digest_record.pop("schema_fingerprint")
    if type(fingerprint) is not str:
        raise SchemaError(f"component fingerprint is invalid for {expected_name}")
    try:
        computed = sha256_bytes(canonical_json_bytes(digest_record))
    except ValueError as error:
        raise SchemaError(
            f"component contains invalid JSON for {expected_name}"
        ) from error
    if fingerprint != computed:
        raise SchemaError(f"component fingerprint mismatch for {expected_name}")

    parameters: list[ParameterSchema] = []
    for parameter in parameter_records:
        name = parameter["name"]
        acir_type = parameter["acir_type"]
        required = parameter["required"]
        constraint = parameter["constraint"]
        if (
            type(name) is not str
            or type(acir_type) is not str
            or type(required) is not bool
            or constraint is not None
            and type(constraint) is not str
        ):
            raise SchemaError(f"invalid static parameter in {expected_name}")
        parameters.append(
            ParameterSchema(name, acir_type, required, parameter["default"], constraint)
        )

    ports: list[PortSchema] = []
    for binding in binding_records:
        if not all(
            type(binding[key]) is str
            for key in ("name", "binding_kind", "acir_type", "direction", "role")
        ) or type(binding["cardinality"]) not in (int, str):
            raise SchemaError(f"invalid binding in {expected_name}")
        ports.append(
            PortSchema(
                binding["name"],
                binding["binding_kind"],
                binding["acir_type"],
                binding["direction"],
                binding["role"],
                binding["cardinality"],
            )
        )

    results: list[ResultSchema] = []
    for result in result_records:
        source_binding = result["source_binding"]
        if (
            type(result["name"]) is not str
            or type(result["acir_type"]) is not str
            or source_binding is not None
            and type(source_binding) is not str
        ):
            raise SchemaError(f"invalid result in {expected_name}")
        results.append(
            ResultSchema(
                result["name"],
                result["acir_type"],
                source_binding,
                result["ownership"],
            )
        )

    names = [port.name for port in ports] + [parameter.name for parameter in parameters]
    if len(names) != len(set(names)) or "name" in names:
        raise SchemaError(f"component signature names collide for {expected_name}")
    return ComponentSchema(
        identity=expected_name,
        fingerprint=fingerprint,
        ports=tuple(ports),
        results=tuple(results),
        parameters=tuple(parameters),
        availability=availability,
        effect_kind=record["effect"]["kind"],
        external_binding=(
            expected_name.replace(".", "_") + "_binding" if external else None
        ),
    )


def signature_for(schema: ComponentSchema) -> inspect.Signature:
    parameters: list[inspect.Parameter] = [
        inspect.Parameter(port.name, inspect.Parameter.POSITIONAL_OR_KEYWORD)
        for port in schema.ports
    ]
    for parameter in schema.parameters:
        default = inspect.Parameter.empty if parameter.required else parameter.default
        parameters.append(
            inspect.Parameter(
                parameter.name, inspect.Parameter.KEYWORD_ONLY, default=default
            )
        )
    parameters.append(
        inspect.Parameter("name", inspect.Parameter.KEYWORD_ONLY, default=None)
    )
    return inspect.Signature(parameters)


class ComponentCallable:
    def __init__(self, schema: ComponentSchema) -> None:
        self.schema = schema
        self.__signature__ = signature_for(schema)
        self.__name__ = schema.identity.rsplit(".", 1)[-1]
        self.__qualname__ = schema.identity

    def __repr__(self) -> str:
        return f"ComponentCallable({self.schema.identity!r})"

    def __call__(self, *args: object, **kwargs: object) -> PendingCall:
        try:
            bound = self.__signature__.bind(*args, **kwargs)
        except TypeError as error:
            raise TypeError(f"ACPY-CALL-003: {error}") from error
        bound.apply_defaults()
        for port in self.schema.ports:
            if not isinstance(bound.arguments[port.name], SymbolicValue):
                raise TypeError(
                    f"ACPY-CALL-003: binding {port.name!r} requires a symbolic value"
                )
        for parameter in self.schema.parameters:
            try:
                validate_ijson_value(bound.arguments[parameter.name])
            except ValueError as error:
                raise TypeError(
                    f"ACPY-CALL-003: parameter {parameter.name!r} is not static"
                ) from error
        instance_name = bound.arguments["name"]
        if instance_name is not None and (
            type(instance_name) is not str or not instance_name
        ):
            raise TypeError("ACPY-CALL-003: instance name must be a non-empty string")
        return PendingCall(self.schema, tuple(bound.arguments.items()))


class SchemaRegistry:
    def __init__(self, schemas: dict[str, ComponentSchema]) -> None:
        self._schemas = MappingProxyType(dict(schemas))

    @classmethod
    def from_catalog(cls, catalog_path: Path, repository: Path) -> "SchemaRegistry":
        root = repository.resolve(strict=True)
        catalog_root = catalog_path.resolve(strict=True).parent
        catalog = _load_json(catalog_root / "catalog.json")
        _exact_keys(
            catalog,
            {"catalog", "version", "contract_epoch", "entries"},
            "stdlib catalog",
        )
        if (
            catalog["catalog"] != "ac"
            or catalog["version"] != "0.1"
            or catalog["contract_epoch"] != "0.4"
        ):
            raise SchemaError("stdlib catalog identity must be ac@0.1")
        entries = _record_list(
            catalog["entries"],
            {"canonical_name", "availability", "schema_path", "schema_fingerprint"},
            "stdlib catalog entries",
        )
        names = [entry["canonical_name"] for entry in entries]
        if (
            any(type(name) is not str for name in names)
            or names != sorted(names)
            or len(names) != len(set(names))
        ):
            raise SchemaError("stdlib catalog names must be unique and ordered")

        schemas: dict[str, ComponentSchema] = {}
        for entry in entries:
            name = entry["canonical_name"]
            availability = entry["availability"]
            schema_path = entry["schema_path"]
            if availability not in ("available", "declared_unavailable"):
                raise SchemaError(f"invalid availability for {name}")
            if type(schema_path) is not str:
                raise SchemaError(f"invalid schema path for {name}")
            path = (root / schema_path).resolve(strict=True)
            try:
                path.relative_to(catalog_root)
            except ValueError as error:
                raise SchemaError(
                    f"component schema escapes stdlib root for {name}"
                ) from error
            schema = _component_schema(_load_json(path), availability, name)
            if entry["schema_fingerprint"] != schema.fingerprint:
                raise SchemaError(f"catalog fingerprint mismatch for {name}")
            schemas[name] = schema
        return cls(schemas)

    def with_component_roots(self, roots: tuple[Path, ...]) -> "SchemaRegistry":
        schemas = dict(self._schemas)
        resolved_roots = sorted(path.resolve() for path in roots)
        for root in resolved_roots:
            if not root.exists():
                continue
            if not root.is_dir():
                raise SchemaError(f"component root is not a directory: {root}")
            for path in sorted(root.rglob("*.component.json")):
                record = _load_json(path)
                identity = record.get("canonical_name")
                if type(identity) is not str:
                    raise SchemaError(f"component schema {path} has no identity")
                schema = _component_schema(record, "available", identity, external=True)
                if identity in schemas:
                    raise SchemaError(f"duplicate component schema {identity}")
                schemas[identity] = schema
        return SchemaRegistry(schemas)

    def schema(self, identity: str) -> ComponentSchema:
        try:
            return self._schemas[identity]
        except KeyError as error:
            raise KeyError(f"ACPY-CALL-001: unknown component {identity!r}") from error

    def callable(self, identity: str) -> ComponentCallable:
        schema = self.schema(identity)
        if schema.availability != "available":
            raise LookupError(f"ACPY-CALL-001: {identity} is declared unavailable")
        return ComponentCallable(schema)

    def candidates(self, short_name: str) -> tuple[ComponentSchema, ...]:
        return tuple(
            schema
            for identity, schema in sorted(self._schemas.items())
            if identity.rsplit(".", 1)[-1] == short_name
            and schema.availability == "available"
        )

    def __len__(self) -> int:
        return len(self._schemas)
